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

#include "scripting/argconv.h"
#include "scripting/toplevel/RegExp.h"

using namespace std;
using namespace lightspark;

RegExp::RegExp(Class_base* c):ASObject(c,T_OBJECT,SUBTYPE_REGEXP),dotall(false),global(false),ignoreCase(false),
	extended(false),multiline(false),lastIndex(0)
{
}

RegExp::RegExp(Class_base* c, const tiny_string& _re):ASObject(c,T_OBJECT,SUBTYPE_REGEXP),dotall(false),global(false),ignoreCase(false),
	extended(false),multiline(false),lastIndex(0),source(_re)
{
}

void RegExp::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_DYNAMIC_NOT_FINAL);
	c->setDeclaredMethodByQName("exec","",Class<IFunction>::getFunction(c->getSystemState(),exec),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("exec",AS3,Class<IFunction>::getFunction(c->getSystemState(),exec),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("test","",Class<IFunction>::getFunction(c->getSystemState(),test),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("test",AS3,Class<IFunction>::getFunction(c->getSystemState(),test),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("toString","",Class<IFunction>::getFunction(c->getSystemState(),_toString),NORMAL_METHOD,true);
	c->prototype->setVariableByQName("toString","",Class<IFunction>::getFunction(c->getSystemState(),_toString),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("exec","",Class<IFunction>::getFunction(c->getSystemState(),exec),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("exec",AS3,Class<IFunction>::getFunction(c->getSystemState(),exec),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("test","",Class<IFunction>::getFunction(c->getSystemState(),test),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("test",AS3,Class<IFunction>::getFunction(c->getSystemState(),test),DYNAMIC_TRAIT);
	REGISTER_GETTER(c,dotall);
	REGISTER_GETTER(c,global);
	REGISTER_GETTER(c,ignoreCase);
	REGISTER_GETTER(c,extended);
	REGISTER_GETTER(c,multiline);
	REGISTER_GETTER_SETTER(c,lastIndex);
	REGISTER_GETTER(c,source);
}

void RegExp::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY_ATOM(RegExp,_constructor)
{
	RegExp* th=obj.as<RegExp>();
	if(argslen > 0 && args[0].is<RegExp>())
	{
		if(argslen > 1 && !args[1].is<Undefined>())
			throwError<TypeError>(kRegExpFlagsArgumentError);
		RegExp *src=args[0].as<RegExp>();
		th->source=src->source;
		th->dotall=src->dotall;
		th->global=src->global;
		th->ignoreCase=src->ignoreCase;
		th->extended=src->extended;
		th->multiline=src->multiline;
		return asAtom::invalidAtom;
	}
	else if(argslen > 0)
		th->source=args[0].toString().raw_buf();
	if(argslen>1 && !args[1].is<Undefined>())
	{
		const tiny_string& flags=args[1].toString();
		for(auto i=flags.begin();i!=flags.end();++i)
		{
			switch(*i)
			{
				case 'g':
					th->global=true;
					break;
				case 'i':
					th->ignoreCase=true;
					break;
				case 'x':
					th->extended=true;
					break;
				case 'm':
					th->multiline=true;
					break;
				case 's':
					// Defined in the Adobe online
					// help but not in ECMA
					th->dotall=true;
					break;
				default:
					break;
			}
		}
	}
	return asAtom::invalidAtom;
}


ASFUNCTIONBODY_ATOM(RegExp,generator)
{
	if(argslen == 0)
	{
		return asAtom::fromObject(Class<RegExp>::getInstanceS(getSys(),""));
	}
	else if(args[0].is<RegExp>())
	{
		ASATOM_INCREF(args[0]);
		return args[0];
	}
	else
	{
		if (argslen > 1)
			LOG(LOG_NOT_IMPLEMENTED, "RegExp generator: flags argument not implemented");
		return asAtom::fromObject(Class<RegExp>::getInstanceS(getSys(),args[0].toString()));
	}
}

ASFUNCTIONBODY_GETTER(RegExp, dotall);
ASFUNCTIONBODY_GETTER(RegExp, global);
ASFUNCTIONBODY_GETTER(RegExp, ignoreCase);
ASFUNCTIONBODY_GETTER(RegExp, extended);
ASFUNCTIONBODY_GETTER(RegExp, multiline);
ASFUNCTIONBODY_GETTER_SETTER(RegExp, lastIndex);
ASFUNCTIONBODY_GETTER(RegExp, source);

ASFUNCTIONBODY_ATOM(RegExp,exec)
{
	RegExp* th=static_cast<RegExp*>(obj.getObject());
	assert_and_throw(argslen==1);
	const tiny_string& arg0=args[0].toString();
	return asAtom::fromObject(th->match(arg0));
}

ASObject *RegExp::match(const tiny_string& str)
{
	pcre* pcreRE = compile();
	if (!pcreRE)
		return getSystemState()->getNullRef();
	int capturingGroups;
	int infoOk=pcre_fullinfo(pcreRE, NULL, PCRE_INFO_CAPTURECOUNT, &capturingGroups);
	if(infoOk!=0)
	{
		pcre_free(pcreRE);
		return getSystemState()->getNullRef();
	}
	//Get information about named capturing groups
	int namedGroups;
	infoOk=pcre_fullinfo(pcreRE, NULL, PCRE_INFO_NAMECOUNT, &namedGroups);
	if(infoOk!=0)
	{
		pcre_free(pcreRE);
		return getSystemState()->getNullRef();
	}
	//Get information about the size of named entries
	int namedSize;
	infoOk=pcre_fullinfo(pcreRE, NULL, PCRE_INFO_NAMEENTRYSIZE, &namedSize);
	if(infoOk!=0)
	{
		pcre_free(pcreRE);
		return getSystemState()->getNullRef();
	}
	struct nameEntry
	{
		uint16_t number;
		char name[0];
	};
	char* entries;
	infoOk=pcre_fullinfo(pcreRE, NULL, PCRE_INFO_NAMETABLE, &entries);
	if(infoOk!=0)
	{
		pcre_free(pcreRE);
		lastIndex=0;
		return getSystemState()->getNullRef();
	}
	pcre_extra extra;
	extra.match_limit_recursion=200;
	extra.flags = PCRE_EXTRA_MATCH_LIMIT_RECURSION;
	int ovector[(capturingGroups+1)*3];
	int offset=global?lastIndex:0;
	int rc=pcre_exec(pcreRE,capturingGroups > 200 ? &extra : NULL, str.raw_buf(), str.numBytes(), offset, 0, ovector, (capturingGroups+1)*3);
	if(rc<0)
	{
		//No matches or error
		pcre_free(pcreRE);
		lastIndex=0;
		return getSystemState()->getNullRef();
	}
	Array* a=Class<Array>::getInstanceSNoArgs(getSystemState());
	//Push the whole result and the captured strings
	for(int i=0;i<capturingGroups+1;i++)
	{
		if(ovector[i*2] >= 0)
			a->push(asAtom::fromObject(abstract_s(getSystemState(), str.substr_bytes(ovector[i*2],ovector[i*2+1]-ovector[i*2]) )));
		else
			a->push(asAtom::fromObject(getSystemState()->getUndefinedRef()));
	}
	a->setVariableByQName("input","",abstract_s(getSystemState(),str),DYNAMIC_TRAIT);

	// pcre_exec returns byte position, so we have to convert it to character position 
	tiny_string tmp = str.substr_bytes(0, ovector[0]);
	int index = tmp.numChars();

	a->setVariableByQName("index","",abstract_i(getSystemState(),index),DYNAMIC_TRAIT);
	for(int i=0;i<namedGroups;i++)
	{
		nameEntry* entry=reinterpret_cast<nameEntry*>(entries);
		uint16_t num=GINT16_FROM_BE(entry->number);
		asAtom captured=a->at(num);
		ASATOM_INCREF(captured);
		a->setVariableAtomByQName(getSystemState()->getUniqueStringId(tiny_string(entry->name, true)),nsNameAndKind(BUILTIN_NAMESPACES::EMPTY_NS),captured,DYNAMIC_TRAIT);
		entries+=namedSize;
	}
	lastIndex=ovector[1];
	pcre_free(pcreRE);
	return a;
}

ASFUNCTIONBODY_ATOM(RegExp,test)
{
	if (!obj.is<RegExp>())
		return asAtom::trueAtom;
	RegExp* th=obj.as<RegExp>();

	const tiny_string& arg0 = args[0].toString();
	pcre* pcreRE = th->compile();
	if (!pcreRE)
		return asAtom::nullAtom;

	int capturingGroups;
	int infoOk=pcre_fullinfo(pcreRE, NULL, PCRE_INFO_CAPTURECOUNT, &capturingGroups);
	if(infoOk!=0)
	{
		pcre_free(pcreRE);
		return asAtom::nullAtom;
	}
	int ovector[(capturingGroups+1)*3];
	
	int offset=(th->global)?th->lastIndex:0;
	pcre_extra extra;
	extra.match_limit_recursion=200;
	extra.flags = PCRE_EXTRA_MATCH_LIMIT_RECURSION;
	int rc = pcre_exec(pcreRE, &extra, arg0.raw_buf(), arg0.numBytes(), offset, 0, ovector, (capturingGroups+1)*3);
	bool ret = (rc >= 0);
	pcre_free(pcreRE);
	return asAtom(ret);
}

ASFUNCTIONBODY_ATOM(RegExp,_toString)
{
	if(Class<RegExp>::getClass(sys)->prototype->getObj() == obj.getObject())
		return asAtom::fromString(sys,"/(?:)/");
	if(!obj.is<RegExp>())
		throw Class<TypeError>::getInstanceS(sys,"RegExp.toString is not generic");

	RegExp* th=obj.as<RegExp>();
	tiny_string ret;
	ret = "/";
	ret += th->source;
	ret += "/";
	if(th->global)
		ret += "g";
	if(th->ignoreCase)
		ret += "i";
	if(th->multiline)
		ret += "m";
	if(th->dotall)
		ret += "s";
	return asAtom::fromObject(abstract_s(sys,ret));
}

pcre* RegExp::compile()
{
	int options = PCRE_UTF8|PCRE_NEWLINE_ANY|PCRE_JAVASCRIPT_COMPAT;
	if(ignoreCase)
		options |= PCRE_CASELESS;
	if(extended)
		options |= PCRE_EXTENDED;
	if(multiline)
		options |= PCRE_MULTILINE;
	if(dotall)
		options|=PCRE_DOTALL;

	const char * error;
	int errorOffset;
	int errorcode;
	pcre* pcreRE=pcre_compile2(source.raw_buf(), options,&errorcode,  &error, &errorOffset,NULL);
	if(error)
	{
		if (errorcode == 64) // invalid pattern in javascript compatibility mode (we try again in normal mode to match flash behaviour)
		{
			options &= ~PCRE_JAVASCRIPT_COMPAT;
			pcreRE=pcre_compile2(source.raw_buf(), options,&errorcode,  &error, &errorOffset,NULL);
		}
		if (error)
			return NULL;
	}
	return pcreRE;
}
