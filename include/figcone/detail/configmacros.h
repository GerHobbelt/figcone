#ifndef FIGCONE_CONFIGMACROS_H
#define FIGCONE_CONFIGMACROS_H

#include "dictcreator.h"
#include "nodecreator.h"
#include "nodelistcreator.h"
#include "paramcreator.h"
#include "paramlistcreator.h"

#define FIGCONE_PARAM(name, type) type name = param<&std::remove_pointer_t<decltype(this)>::name>(#name)
#define FIGCONE_NODE(name, type) type name = node<&std::remove_pointer_t<decltype(this)>::name>(#name)
#define FIGCONE_COPY_NODELIST(name, listType)                                                                          \
    listType name = copyNodeList<&std::remove_pointer_t<decltype(this)>::name>(#name)
#define FIGCONE_NODELIST(name, listType) listType name = nodeList<&std::remove_pointer_t<decltype(this)>::name>(#name)
#define FIGCONE_PARAMLIST(name, listType) listType name = paramList<&std::remove_pointer_t<decltype(this)>::name>(#name)
#define FIGCONE_DICT(name, mapType) mapType name = dict<&std::remove_pointer_t<decltype(this)>::name>(#name)

#endif //FIGCONE_CONFIGMACROS_H