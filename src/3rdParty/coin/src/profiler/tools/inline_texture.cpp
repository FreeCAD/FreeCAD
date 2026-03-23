#include <Inventor/SoDB.h>
#include <Inventor/SoInput.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoTexture2.h>
#include <Inventor/actions/SoSearchAction.h>
#include <Inventor/actions/SoWriteAction.h>

/*
  rolvs 20071107
  This is ripped from the Texture2 documentation. Build as inline_texture, add to path, before running
  inventify_texturenames.sh. Used to pull inn the textures in ../spheremaps to the SoNodeVisualize.
*/

int
main(void)
{
  SoDB::init();

  SoInput in;
  SoSeparator * root = SoDB::readAll(&in);
  if (!root) { exit(1); }

  root->ref();

  SoSearchAction searchaction;
  searchaction.setType(SoTexture2::getClassTypeId());
  searchaction.setSearchingAll(TRUE);
  searchaction.setInterest(SoSearchAction::ALL);

  searchaction.apply(root);

  const SoPathList & pl = searchaction.getPaths();
  for (int i=0; i < pl.getLength(); i++) {
    SoFullPath * fp = (SoFullPath *)pl[i];
    SoTexture2 * tex = (SoTexture2 *)fp->getTail();
    assert(tex->getTypeId() == SoTexture2::getClassTypeId());
    tex->image.touch();
  }

  SoWriteAction wa;
  wa.apply(root);

  root->unref();

  return 0;
}
