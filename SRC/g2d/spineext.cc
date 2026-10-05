
#include "spineext.h"
#include <SpineString.h>
#include "rsrcfile.h"
#include "assetloader.h"

_G2D_NAMESPACE_BEGIN_

char *SpineExt::_readFile(const spine::String &path, int *length)
{
    char* pRes = NULL;
    int nSize;
    auto pBytes = AssetLoader::instance().getLoadedFile(path.buffer(), nSize);
    assert(pBytes);
    if (pBytes)
    {
        const char* ptt = (const char*)pBytes;
        *length = nSize;
        return (char*)pBytes;
    }
    return pRes;
}

_G2D_NAMESPACE_END_

spine::SpineExtension *spine::getDefaultExtension() 
{
   return new _G2D_NS_::SpineExt();
}

