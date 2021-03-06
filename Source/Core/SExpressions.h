#pragma once

#include "Core/Core.h"
#include "Core/MemoryArena.h"

#ifdef _WIN32
	#pragma warning (disable:4512)	// assignment operator could not be generated
#endif

namespace SExp
{
	// The type of a S-expression node.
	enum class NodeType : uint8
	{
		Symbol,
		String,
		UnindexedSymbol,
		Name,
		SignedInt,
		UnsignedInt,
		Float,
		Error,
		Tree,
		Attribute // A tree with two children implicitly separated by an '='
	};

	// A node in a tree of S-expressions
	struct Node
	{
		// The type of the node. Determines how to interpret the union.
		NodeType type;
		union
		{
			const char* error;
			uintptr symbol;
			const char* string;
			int64 i64;
			uint64 u64;
			float64 f64;
			Node* children;
		};
		union
		{
			// If type==NodeType::String, contains the length of the string.
			// string[stringLength] will be zero, but stringLength may not be the first zero in the string.
			size_t stringLength;
			// If type==NodeType::Float, contains a 32-bit float representation of the node.
			float32 f32;
		};
		// The next node with the same parent.
		Node* nextSibling;
		// The start of this node in the source file.
		Core::TextFileLocus startLocus;
		// The end of this node in the source file.
		Core::TextFileLocus endLocus;

		Node(const Core::TextFileLocus& inStartLocus = Core::TextFileLocus(),NodeType inType = NodeType::Tree)
		:	type(inType)
		,	children(nullptr)
		,	stringLength(0)
		,	nextSibling(nullptr)
		,	startLocus(inStartLocus)
		,	endLocus(inStartLocus)
		{}
	};

	// Iterates over sibling nodes in a S-expression tree.
	struct NodeIt
	{
		Node* node;
		Core::TextFileLocus previousLocus;

		NodeIt(): node(nullptr) {}
		explicit NodeIt(Node* inNode,Core::TextFileLocus inPreviousLocus = Core::TextFileLocus())
		: node(inNode), previousLocus(inPreviousLocus) {}

		NodeIt& operator++()
		{
			if(node)
			{
				previousLocus = node->endLocus;
				node = node->nextSibling;
			}
			return *this;
		}
		NodeIt operator++(int)
		{
			NodeIt copy = *this;
			++(*this);
			return copy;
		}
		
		NodeIt getChildIt() const
		{
			assert(node->type == NodeType::Tree || node->type == NodeType::Attribute);
			return NodeIt(node->children,node->startLocus);
		}

		Node* operator->() const { return node; }
		operator Node*() const { return node; }
		operator bool() const { return node != nullptr; }
	};

	// An output stream for S-expression nodes.
	struct NodeOutputStream
	{
		// Used to distinguish an unindexed symbol from a quoted string, which may contain zeroes other than its terminator.
		struct StringAtom
		{
			const char* string;
			size_t length;
			StringAtom(const char* inString,size_t inLength) : string(inString), length(inLength) {}
			StringAtom(const std::string& inString) : string(inString.c_str()), length(inString.length()) {}
		};
		
		NodeOutputStream(Memory::Arena& inArena): arena(inArena), rootNode(nullptr), nextNodeLink(&rootNode) {}

		// Appends a subtree node, and sets the stream to append to its children.
		void enterSubtree()
		{
			Node* subtreeNode = new(arena) Node();
			*nextNodeLink = subtreeNode;
			nextNodeLink = &subtreeNode->children;
		}

		// Appends an attribute node, and sets the stream to append to its children.
		void enterAttribute()
		{
			Node* subtreeNode = new(arena) Node();
			subtreeNode->type = NodeType::Attribute;
			*nextNodeLink = subtreeNode;
			nextNodeLink = &subtreeNode->children;
		}

		// Append a substream's nodes.
		NodeOutputStream& operator<<(const NodeOutputStream& substream)
		{
			append(substream.getRoot());
			return *this;
		}
		// Append a S-expression node.
		NodeOutputStream& operator<<(Node* node)
		{
			append(node);
			return *this;
		}
		// Append an unindexed symbol.
		NodeOutputStream& operator<<(const char* string)
		{
			auto stringLength = strlen(string);
			auto symbolNode = new(arena) Node();
			symbolNode->type = SExp::NodeType::UnindexedSymbol;
			symbolNode->string = arena.copyToArena(string,stringLength + 1);;
			symbolNode->stringLength = stringLength;
			append(symbolNode);
			return *this;
		}
		NodeOutputStream& operator<<(const std::string& string) { return *this << string.c_str(); }
		NodeOutputStream& operator<<(bool b) { appendInt(b ? 1 : 0); return *this; }
		NodeOutputStream& operator<<(uint8 i) { appendInt(i); return *this; }
		NodeOutputStream& operator<<(uint16 i) { appendInt(i); return *this; }
		NodeOutputStream& operator<<(uint32 i) { appendInt(i); return *this; }
		NodeOutputStream& operator<<(uint64 i) { appendInt(i); return *this; }
		NodeOutputStream& operator<<(intptr i) { appendInt(i); return *this; }
		NodeOutputStream& operator<<(float64 f)
		{
			auto decimalNode = new(arena) Node();
			decimalNode->type = SExp::NodeType::Float;
			decimalNode->f64 = f;
			decimalNode->f32 = (float32)f;
			append(decimalNode);
			return *this;
		}
		
		NodeOutputStream& operator<<(const StringAtom& string)
		{
			auto intNode = new(arena) Node();
			intNode->type = SExp::NodeType::String;
			intNode->string = string.string;
			intNode->stringLength = string.length;
			append(intNode);
			return *this;
		}
		
		Memory::Arena& getArena() { return arena; }
		Node* getRoot() const { return rootNode; }

	private:
		Memory::Arena& arena;
		Node* rootNode;
		Node** nextNodeLink;

		void append(Node* node)
		{
			if(node)
			{
				*nextNodeLink = node;
				while(node->nextSibling) { node = node->nextSibling; };
				nextNodeLink = &node->nextSibling;
			}
		}

		template<typename T> void appendInt(T i)
		{
			auto intNode = new(arena) Node();
			intNode->type = i < 0 ? SExp::NodeType::SignedInt : SExp::NodeType::UnsignedInt;
			intNode->u64 = i < 0 ? (uint64)-(int64)i : i;
			append(intNode);
		}
	};

	// Parses a S-expression tree from a string, allocating nodes from arena, and using symbolIndexMap to map symbols to indices.
	typedef std::map<const char*,uintptr,Core::StringCompareFunctor> SymbolIndexMap;
	CORE_API Node* parse(const char* string,Memory::Arena& arena,const SymbolIndexMap& symbolIndexMap);

	// Prints a S-expression tree to a string.
	CORE_API std::string print(SExp::Node* rootNode,const char* symbolStrings[]);
}
