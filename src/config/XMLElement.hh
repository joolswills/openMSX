#ifndef XMLELEMENT_HH
#define XMLELEMENT_HH

#include "serialize_meta.hh"
#include <utility>
#include <string>
#include <vector>
#include <memory>

namespace openmsx {

class FileContext;

class XMLElement
{
public:
	//
	// Basic functions
	//

	// Construction.
	//  (copy, assign, move, destruct are default)
	XMLElement() = default;
	explicit XMLElement(std::string name_)
		: name(std::move(name_)) {}
	XMLElement(std::string name_, std::string data_)
		: name(std::move(name_)), data(std::move(data_)) {}

	// name
	const std::string& getName() const { return name; }
	void setName(std::string name_) { name = std::move(name_); }
	void clearName() { name.clear(); }

	// data
	const std::string& getData() const { return data; }
	void setData(std::string data_) {
		assert(children.empty()); // no mixed-content elements
		data = std::move(data_);
	}

	// attribute
	void addAttribute(std::string name, std::string value);
	void setAttribute(string_view name, std::string value);
	void removeAttribute(string_view name);
	bool hasAttribute(string_view name) const;
	const std::string& getAttribute(string_view attName) const;
	string_view getAttribute(string_view attName,
	                        string_view defaultValue) const;
	// Returns ptr to attribute value, or nullptr when not found.
	const std::string* findAttribute(string_view attName) const;

	// child
	using Children = std::vector<XMLElement>;
	//  note: returned XMLElement& is validated on the next addChild() call
	XMLElement& addChild(std::string name);
	XMLElement& addChild(std::string name, std::string data);
	void removeChild(const XMLElement& child);
	const Children& getChildren() const { return children; }
	bool hasChildren() const { return !children.empty(); }

	//
	// Convenience functions
	//

	// attribute
	bool getAttributeAsBool(string_view attName,
	                        bool defaultValue = false) const;
	int getAttributeAsInt(string_view attName,
	                      int defaultValue = 0) const;
	bool findAttributeInt(string_view attName,
	                      unsigned& result) const;

	// child
	const XMLElement* findChild(string_view name) const;
	XMLElement* findChild(string_view name);
	const XMLElement& getChild(string_view name) const;
	XMLElement& getChild(string_view name);

	const XMLElement* findChildWithAttribute(
		string_view name, string_view attName,
		string_view attValue) const;
	XMLElement* findChildWithAttribute(
		string_view name, string_view attName,
		string_view attValue);
	const XMLElement* findNextChild(string_view name,
	                                size_t& fromIndex) const;

	std::vector<const XMLElement*> getChildren(string_view name) const;

	XMLElement& getCreateChild(string_view name,
	                           string_view defaultValue = {});
	XMLElement& getCreateChildWithAttribute(
		string_view name, string_view attName,
		string_view attValue);

	const std::string& getChildData(string_view name) const;
	string_view getChildData(string_view name,
	                        string_view defaultValue) const;
	bool getChildDataAsBool(string_view name,
	                        bool defaultValue = false) const;
	int getChildDataAsInt(string_view name,
	                      int defaultValue = 0) const;
	void setChildData(string_view name, std::string value);

	void removeAllChildren();

	// various
	std::string dump() const;
	static std::string XMLEscape(const std::string& str);

	template<typename Archive>
	void serialize(Archive& ar, unsigned version);

	// For backwards compatibility with older savestates
	static std::unique_ptr<FileContext> getLastSerializedFileContext();

private:
	using Attribute = std::pair<std::string, std::string>;
	using Attributes = std::vector<Attribute>;
	Attributes::iterator getAttributeIter(string_view name);
	Attributes::const_iterator getAttributeIter(string_view name) const;
	void dump(std::string& result, unsigned indentNum) const;

	std::string name;
	std::string data;
	Children children;
	Attributes attributes;
};
SERIALIZE_CLASS_VERSION(XMLElement, 2);

} // namespace openmsx

#endif
