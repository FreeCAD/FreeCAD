// Uses introspection to list out all nodes, with all fields and the
// enum values of SoSFEnum, SoMFEnum, SoSFBitMask and SoMFBitMask type
// fields.

// FIXME: sort node names alphabetically before printing, and we have
// a tool for checking when different types of nodes were added into
// TGS Inventor. 20030623 mortene.

#include <stdio.h>
#include <Inventor/SoDB.h>
#include <Inventor/nodekits/SoNodeKit.h>
#include <Inventor/SoInteraction.h>
#include <Inventor/SoLists.h>
#include <Inventor/nodes/SoNode.h>
#include <Inventor/fields/SoSFEnum.h>
#include <Inventor/fields/SoMFEnum.h>

template <class Type>
void list_enums(const Type * f)
{
  const unsigned int nrenums = f->getNumEnums();
  for (unsigned int k=0; k < nrenums; k++) {
    SbName name;
    const int val = f->getEnum(k, name);
    (void)fprintf(stdout, "    %s == %d\n",
                  name.getString(), val);
  }
}

int
main(void)
{
  SoDB::init();
  SoNodeKit::init();
  SoInteraction::init();

  SoTypeList tl;
  const unsigned int n = SoType::getAllDerivedFrom(SoNode::getClassTypeId(), tl);
  for (unsigned int i=0; i < n; i++) {
    (void)fprintf(stdout, "%s", tl[i].getName().getString());

    SoFieldContainer * fc = (SoFieldContainer *)
      (tl[i].canCreateInstance() ? tl[i].createInstance() : NULL);

    if (fc == NULL) {
      (void)fprintf(stdout, "  (abstract)\n");
      continue;
    }

    (void)fprintf(stdout, "\n");

    SoFieldList fl;
    const unsigned int nrf = fc->getAllFields(fl);
    for (unsigned int j=0; j < nrf; j++) {
      SoField * f = fl[j];
      SoType ftype = f->getTypeId();
      SbName fname;
      f->getContainer()->getFieldName(f, fname);
      (void)fprintf(stdout, "  %s (%s)\n",
                    fname.getString(),
                    ftype.getName().getString());

      if (ftype.isDerivedFrom(SoSFEnum::getClassTypeId())) {
        list_enums((SoSFEnum *)f);
      }
      else if (ftype.isDerivedFrom(SoMFEnum::getClassTypeId())) {
        list_enums((SoMFEnum *)f);
      }
    }
  }

  return 0;
}
