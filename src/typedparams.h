/*
 * typedparams.h: Helpers for translating virTypedParameter to/from assoc arrays
 *
 * See COPYING for the license of this software
 */

#ifndef __TYPEDPARAMS_H__
# define __TYPEDPARAMS_H__

#include <libvirt/libvirt.h>

typedef struct {
    const char *name;
    virTypedParameterType type;
} virPHPTypedParamsHint;

int
parseTypedParameter(zval *zend_params,
                    virTypedParameterPtr *paramsRet,
                    int *nparamsRet,
                    virPHPTypedParamsHint *hint,
                    int nhints);

#endif /* __TYPEDPARAMS_H__ */
