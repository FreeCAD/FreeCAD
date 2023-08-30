/***************************************************************************
 *   Copyright (c) 2011 Jürgen Riegel <juergen.riegel@web.de>              *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#include "PreCompiled.h"

#ifndef _PreComp_
# include <cassert>
#endif

/// Here the FreeCAD includes sorted by Base,App,Gui......
#include "Type.h"
#include "Exception.h"
#include "Interpreter.h"
#include "Console.h"


using namespace Base;
using namespace std;


struct Base::TypeData
{
  TypeData(const char *theName,
           const Type type = Type::badType(),
           const Type theParent = Type::badType(),
           Type::instantiationMethod method = nullptr
          ):name(theName),parent(theParent),type(type),instMethod(method) { }

  std::string name;
  Type parent;
  Type type;
  Type::instantiationMethod instMethod;
};

map<string,unsigned int> Type::typemap;
vector<TypeData*>        Type::typedata;
set<string>              Type::loadModuleSet;

void *Type::createInstance()
{
  instantiationMethod method = typedata[index]->instMethod;
  return method ? (*method)() : nullptr;
}


void *Type::createInstanceByName(const char* TypeName, bool bLoadModule)
{
  // if not already, load the module
  if (bLoadModule)
    importModule(TypeName);

  // now the type should be in the type map
  Type t = fromName(TypeName);
  if (t == badType())
    return nullptr;

  return t.createInstance();
}

void Type::importModule(const char* TypeName)
{
  // cut out the module name
  string Mod = getModuleName(TypeName);
  // ignore base modules
  if (Mod != "App" && Mod != "Gui" && Mod != "Base") {
    // remember already loaded modules
    set<string>::const_iterator pos = loadModuleSet.find(Mod);
    if (pos == loadModuleSet.end()) {
      Interpreter().loadModule(Mod.c_str());
#ifdef FC_LOGLOADMODULE
      Console().Log("Act: Module %s loaded through class %s \n",Mod.c_str(),TypeName);
#endif
      loadModuleSet.insert(Mod);
    }
  }
}

string Type::getModuleName(const char* ClassName)
{
  string temp(ClassName);
  std::string::size_type pos = temp.find_first_of("::");

  if (pos != std::string::npos)
    return {temp,0,pos};
  return {};
}

Type Type::badType()
{
  Type bad;
  bad.index = 0;
  return bad;
}


const Type Type::createType(const Type parent, const char *name, instantiationMethod method)
{
  Type newType;
  newType.index = static_cast<unsigned int>(Type::typedata.size());
  TypeData * typeData = new TypeData(name, newType, parent,method);
  Type::typedata.push_back(typeData);

  // add to dictionary for fast lookup
  Type::typemap[name] = newType.getKey();

  return newType;
}


void Type::init()
{
  assert(Type::typedata.size() == 0);


  Type::typedata.push_back(new TypeData("BadType"));
  Type::typemap["BadType"] = 0;


}

void Type::destruct()
{
  for(auto it : typedata)
    delete it;
  typedata.clear();
  typemap.clear();
  loadModuleSet.clear();
}

Type Type::fromName(const char *name)
{
  std::map<std::string,unsigned int>::const_iterator pos;

  pos = typemap.find(name);
  if (pos != typemap.end())
    return typedata[pos->second]->type;
  else
    return Type::badType();
}

Type Type::fromKey(unsigned int key)
{
  if (key < typedata.size())
    return typedata[key]->type;
  else
    return Type::badType();
}

const char *Type::getName() const
{
  return typedata[index]->name.c_str();
}

const Type Type::getParent() const
{
  return typedata[index]->parent;
}

bool Type::isDerivedFrom(const Type type) const
{

  Type temp(*this);
  do {
    if (temp == type)
      return true;
    temp = temp.getParent();
  } while (temp != badType());

  return false;
}

int Type::getAllDerivedFrom(const Type type, std::vector<Type> & List)
{
  int cnt = 0;

  for(auto it : typedata)
  {
    if (it->type.isDerivedFrom(type))
    {
      List.push_back(it->type);
      cnt++;
    }
  }
  return cnt;
}

int Type::getNumTypes()
{
  return static_cast<int>(typedata.size());
}

Type Type::getTypeIfDerivedFrom(const char* name , const Type parent, bool bLoadModule)
{
  if (bLoadModule)
    importModule(name);

  Type type = fromName(name);

  if (type.isDerivedFrom(parent))
    return type;
  else
    return Type::badType();
}
