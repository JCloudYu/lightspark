/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2013  Alessandro Pignotti (a.pignotti@sssup.it)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************/

#include "scripting/abc.h"
#include "asobject.h"
#include "scripting/class.h"
#include "compat.h"
#include "parsing/amf3_generator.h"
#include "scripting/argconv.h"
#include "scripting/flash/errors/flasherrors.h"
#include "scripting/flash/utils/Dictionary.h"

using namespace std;
using namespace lightspark;

Dictionary::Dictionary(Class_base* c):ASObject(c),
	data(std::less<dictType::key_type>(), reporter_allocator<dictType::value_type>(c->memoryAccount))
{
}

void Dictionary::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_DYNAMIC_NOT_FINAL);
	c->prototype->setVariableByQName("toJSON",AS3,Class<IFunction>::getFunction(c->getSystemState(),_toJSON),DYNAMIC_TRAIT);
}

void Dictionary::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY(Dictionary,_constructor)
{
	bool weak = false;
	ARG_UNPACK(weak, false);
	if (weak)
		LOG(LOG_NOT_IMPLEMENTED,"Dictionary:weak keys not implemented");
	return NULL;
}

ASFUNCTIONBODY(Dictionary,_toJSON)
{
	return abstract_s(getSys(),"Dictionary");
}

Dictionary::dictType::iterator Dictionary::findKey(ASObject *o)
{
	Dictionary::dictType::iterator it = data.begin();
	for(; it!=data.end(); ++it)
	{
		if (it->first->isEqualStrict(o))
			return it;
	}

	return data.end();
}

void Dictionary::setVariableByMultiname_i(const multiname& name, int32_t value)
{
	assert_and_throw(implEnable);
	asAtom v(value);
	Dictionary::setVariableByMultiname(name,v,CONST_NOT_ALLOWED);
}

void Dictionary::setVariableByMultiname(const multiname& name, asAtom& o, CONST_ALLOWED_FLAG allowConst)
{
	assert_and_throw(implEnable);
	if(name.name_type==multiname::NAME_OBJECT)
	{
		multiname tmpname(NULL);
		tmpname.ns.push_back(nsNameAndKind(getSystemState(),"",NAMESPACE));
		switch (name.name_o->getObjectType())
		{
			case T_BOOLEAN:
			case T_INTEGER:
				tmpname.name_type=multiname::NAME_INT;
				tmpname.name_i = name.name_o->toInt();
				ASObject::setVariableByMultiname(tmpname, o, allowConst);
				return;
			case T_UINTEGER:
				tmpname.name_type=multiname::NAME_UINT;
				tmpname.name_ui = name.name_o->toUInt();
				ASObject::setVariableByMultiname(tmpname, o, allowConst);
				return;
			case T_NUMBER:
				tmpname.name_type=multiname::NAME_NUMBER;
				tmpname.name_d = name.name_o->toNumber();
				ASObject::setVariableByMultiname(tmpname, o, allowConst);
				return;
			case T_STRING:
				tmpname.name_type=multiname::NAME_STRING;
				tmpname.name_s_id = name.name_o->toStringId();
				ASObject::setVariableByMultiname(tmpname, o, allowConst);
				return;
			default:
				break;
		}
		name.name_o->incRef();
		_R<ASObject> name_o(name.name_o);

		Dictionary::dictType::iterator it=findKey(name_o.getPtr());
		if(it!=data.end())
			it->second=_MR(o.toObject(getSystemState()));
		else
			data.insert(make_pair(name_o,_MR(o.toObject(getSystemState()))));
	}
	else
	{
		//Primitive types _must_ be handled by the normal ASObject path
		//REFERENCE: Dictionary Object on AS3 reference
		assert(name.name_type==multiname::NAME_STRING ||
			name.name_type==multiname::NAME_INT ||
			name.name_type==multiname::NAME_UINT ||
			name.name_type==multiname::NAME_NUMBER);
		ASObject::setVariableByMultiname(name, o, allowConst);
	}
}

bool Dictionary::deleteVariableByMultiname(const multiname& name)
{
	assert_and_throw(implEnable);

	if(name.name_type==multiname::NAME_OBJECT)
	{
		multiname tmpname(NULL);
		tmpname.ns.push_back(nsNameAndKind(getSystemState(),"",NAMESPACE));
		switch (name.name_o->getObjectType())
		{
			case T_BOOLEAN:
			case T_INTEGER:
				tmpname.name_type=multiname::NAME_INT;
				tmpname.name_i = name.name_o->toInt();
				return ASObject::deleteVariableByMultiname(tmpname);
			case T_UINTEGER:
				tmpname.name_type=multiname::NAME_UINT;
				tmpname.name_ui = name.name_o->toUInt();
				return ASObject::deleteVariableByMultiname(tmpname);
			case T_NUMBER:
				tmpname.name_type=multiname::NAME_NUMBER;
				tmpname.name_d = name.name_o->toNumber();
				return ASObject::deleteVariableByMultiname(tmpname);
			case T_STRING:
				tmpname.name_type=multiname::NAME_STRING;
				tmpname.name_s_id = name.name_o->toStringId();
				return ASObject::deleteVariableByMultiname(tmpname);
			default:
				break;
		}
		name.name_o->incRef();
		_R<ASObject> name_o(name.name_o);

		Dictionary::dictType::iterator it=findKey(name_o.getPtr());
		if(it != data.end())
		{
			data.erase(it);
			return true;
		}
		return false;
	}
	else
	{
		//Primitive types _must_ be handled by the normal ASObject path
		//REFERENCE: Dictionary Object on AS3 reference
		assert(name.name_type==multiname::NAME_STRING ||
			name.name_type==multiname::NAME_INT ||
			name.name_type==multiname::NAME_UINT ||
			name.name_type==multiname::NAME_NUMBER);
		return ASObject::deleteVariableByMultiname(name);
	}
}

asAtom Dictionary::getVariableByMultiname(const multiname& name, GET_VARIABLE_OPTION opt)
{
	if((opt & ASObject::SKIP_IMPL)==0 && implEnable)
	{
		if(name.name_type==multiname::NAME_OBJECT)
		{
			multiname tmpname(NULL);
			tmpname.ns.push_back(nsNameAndKind(getSystemState(),"",NAMESPACE));
			switch (name.name_o->getObjectType())
			{
				case T_BOOLEAN:
				case T_INTEGER:
					tmpname.name_type=multiname::NAME_INT;
					tmpname.name_i = name.name_o->toInt();
					return ASObject::getVariableByMultiname(tmpname, opt);
				case T_UINTEGER:
					tmpname.name_type=multiname::NAME_UINT;
					tmpname.name_ui = name.name_o->toUInt();
					return ASObject::getVariableByMultiname(tmpname, opt);
				case T_NUMBER:
					tmpname.name_type=multiname::NAME_NUMBER;
					tmpname.name_d = name.name_o->toNumber();
					return ASObject::getVariableByMultiname(tmpname, opt);
				case T_STRING:
					tmpname.name_type=multiname::NAME_STRING;
					tmpname.name_s_id = name.name_o->toStringId();
					return ASObject::getVariableByMultiname(tmpname, opt);
				default:
					break;
			}
			name.name_o->incRef();
			_R<ASObject> name_o(name.name_o);

			Dictionary::dictType::iterator it=findKey(name_o.getPtr());
			if(it != data.end())
			{
				it->second->incRef();
				return asAtom::fromObject(it->second.getPtr());
			}
			else
				return asAtom::invalidAtom;
		}
		else
		{
			//Primitive types _must_ be handled by the normal ASObject path
			//REFERENCE: Dictionary Object on AS3 reference
			assert(name.name_type==multiname::NAME_STRING ||
				name.name_type==multiname::NAME_INT ||
				name.name_type==multiname::NAME_UINT ||
				name.name_type==multiname::NAME_NUMBER);
			return ASObject::getVariableByMultiname(name, opt);
		}
	}
	//Try with the base implementation
	return ASObject::getVariableByMultiname(name, opt);
}

bool Dictionary::hasPropertyByMultiname(const multiname& name, bool considerDynamic, bool considerPrototype)
{
	if(considerDynamic==false)
	{
		if(name.name_type==multiname::NAME_OBJECT)
			return false;
		return ASObject::hasPropertyByMultiname(name, considerDynamic, considerPrototype);
	}
	if (!isConstructed())
		return false;

	if(name.name_type==multiname::NAME_OBJECT)
	{
		multiname tmpname(NULL);
		tmpname.ns.push_back(nsNameAndKind(getSystemState(),"",NAMESPACE));
		switch (name.name_o->getObjectType())
		{
			case T_BOOLEAN:
			case T_INTEGER:
				tmpname.name_type=multiname::NAME_INT;
				tmpname.name_i = name.name_o->toInt();
				return ASObject::hasPropertyByMultiname(tmpname, considerDynamic, considerPrototype);
			case T_UINTEGER:
				tmpname.name_type=multiname::NAME_UINT;
				tmpname.name_ui = name.name_o->toUInt();
				return ASObject::hasPropertyByMultiname(tmpname, considerDynamic, considerPrototype);
			case T_NUMBER:
				tmpname.name_type=multiname::NAME_NUMBER;
				tmpname.name_d = name.name_o->toNumber();
				return ASObject::hasPropertyByMultiname(tmpname, considerDynamic, considerPrototype);
			case T_STRING:
				tmpname.name_type=multiname::NAME_STRING;
				tmpname.name_s_id = name.name_o->toStringId();
				return ASObject::hasPropertyByMultiname(tmpname, considerDynamic, considerPrototype);
			default:
				break;
		}

		name.name_o->incRef();
		_R<ASObject> name_o(name.name_o);

		Dictionary::dictType::iterator it=findKey(name_o.getPtr());
		return it != data.end();
	}
	else
	{
		//Primitive types _must_ be handled by the normal ASObject path
		//REFERENCE: Dictionary Object on AS3 reference
		assert(name.name_type==multiname::NAME_STRING ||
			name.name_type==multiname::NAME_INT ||
			name.name_type==multiname::NAME_UINT ||
			name.name_type==multiname::NAME_NUMBER);
		return ASObject::hasPropertyByMultiname(name, considerDynamic, considerPrototype);
	}
}

uint32_t Dictionary::nextNameIndex(uint32_t cur_index)
{
	assert_and_throw(implEnable);
	if(cur_index<data.size())
		return cur_index+1;
	else
	{
		//Fall back on object properties
		uint32_t ret=ASObject::nextNameIndex(cur_index-data.size());
		if(ret==0)
			return 0;
		else
			return ret+data.size();

	}
}

asAtom Dictionary::nextName(uint32_t index)
{
	assert_and_throw(implEnable);
	if(index<=data.size())
	{
		map<_R<ASObject>,_R<ASObject> >::iterator it=data.begin();
		for(unsigned int i=1;i<index;i++)
			++it;
		it->first->incRef();
		return asAtom::fromObject(it->first.getPtr());
	}
	else
	{
		//Fall back on object properties
		return ASObject::nextName(index-data.size());
	}
}

asAtom Dictionary::nextValue(uint32_t index)
{
	assert_and_throw(implEnable);
	if(index<=data.size())
	{
		map<_R<ASObject>,_R<ASObject> >::iterator it=data.begin();
		for(unsigned int i=1;i<index;i++)
			++it;

		it->second->incRef();
		return asAtom::fromObject(it->second.getPtr());
	}
	else
	{
		//Fall back on object properties
		return ASObject::nextValue(index-data.size());
	}
}

tiny_string Dictionary::toString()
{
	std::stringstream retstr;
	retstr << "{";
	map<_R<ASObject>,_R<ASObject> >::iterator it=data.begin();
	while(it != data.end())
	{
		if(it != data.begin())
			retstr << ", ";
		retstr << "{" << it->first->toString() << ", " << it->second->toString() << "}";
		++it;
	}
	retstr << "}";

	return retstr.str();
}


void Dictionary::serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
				std::map<const ASObject*, uint32_t>& objMap,
				std::map<const Class_base*, uint32_t>& traitsMap)
{
	if (out->getObjectEncoding() == ObjectEncoding::AMF0)
	{
		LOG(LOG_NOT_IMPLEMENTED,"serializing Dictionary in AMF0 not implemented");
		return;
	}
	assert_and_throw(objMap.find(this)==objMap.end());
	out->writeByte(dictionary_marker);
	//Check if the dictionary has been already serialized
	auto it=objMap.find(this);
	if(it!=objMap.end())
	{
		//The least significant bit is 0 to signal a reference
		out->writeU29(it->second << 1);
	}
	else
	{
		//Add the dictionary to the map
		objMap.insert(make_pair(this, objMap.size()));

		uint32_t count = 0;
		uint32_t tmp;
		while ((tmp = nextNameIndex(count)) != 0)
		{
			count = tmp;
		}
		assert_and_throw(count<0x20000000);
		uint32_t value = (count << 1) | 1;
		out->writeU29(value);
		out->writeByte(0x00); // TODO handle weak keys
		
		tmp = 0;
		while ((tmp = nextNameIndex(tmp)) != 0)
		{
			nextName(tmp).toObject(getSystemState())->serialize(out, stringMap, objMap, traitsMap);
			nextValue(tmp).toObject(getSystemState())->serialize(out, stringMap, objMap, traitsMap);
		}
	}
}

