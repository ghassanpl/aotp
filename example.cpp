#include <string>
#include <map>
#include <vector>
#include <typeindex>
#include <cassert>

#include "AOTP.h"

struct Object
{
	size_t HashCache = 0;
	bool HasHashCache = false;

	virtual size_t Hash() = 0;
};

struct CommitObject : Object
{
	std::vector<size_t> Parents;
	size_t Tree;
	std::string Message;

	virtual size_t Hash() override
	{
		if (!HasHashCache)
		{
			auto str = std::to_string(Tree) + "\n";
			for (auto& parent : Parents)
				str += std::to_string(parent) + "\n";
			str += "\n" + Message;

			HashCache = std::hash<std::string>()(str);
			HasHashCache = true;
		}

		return HashCache;
	}
};

struct FileObject : Object
{
	std::string Value;

	virtual size_t Hash() override
	{
		if (!HasHashCache)
		{
			HashCache = std::hash<std::string>()(Value);
			HasHashCache = true;
		}

		return HashCache;
	}
};

struct TreeObject : Object
{
	std::map<std::string, size_t> Children;

	virtual size_t Hash() override
	{
		if (!HasHashCache)
		{
			std::string str = "";
			for (auto& child : Children)
				str += child.first + " " + std::to_string(child.second) + "\n";

			HashCache = std::hash<std::string>()(str);
			HasHashCache = true;
		}

		return HashCache;
	}
};

typedef variant<Object, CommitObject, FileObject, TreeObject> ObjectVariant;

bool TestAOTP()
{
	ObjectVariant v;
	v.set<FileObject>();
	v.get<FileObject>().Value = "woo";
	return v->Hash() == std::hash<std::string>()("woo");
}

int main()
{
	assert(TestAOTP());
	return 0;
}