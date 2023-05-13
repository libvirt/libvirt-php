/*
 * typedparams.c: Helpers for translating virTypedParameter to/from assoc arrays
 *
 * See COPYING for the license of this software
 */

#include <config.h>

#include <php.h>

#include "typedparams.h"
#include "util.h"

DEBUG_INIT("typedparams");

int
parseTypedParameter(zval *zend_params,
                    virTypedParameterPtr *paramsRet,
                    int *nparamsRet,
                    virPHPTypedParamsHint *hint,
                    int nhints)
{
    HashTable *arr_hash = NULL;
    HashPosition pointer;
    zval *data;
    virTypedParameterPtr params = NULL;
    int nparams = 0;
    int maxparams = 0;
    int ret = -1;

    if (!zend_params)
        return 0;

    arr_hash = Z_ARRVAL_P(zend_params);

    VIRT_FOREACH(arr_hash, pointer, data) {
        php_libvirt_hash_key_info key;
        int i;

        VIRT_HASH_CURRENT_KEY_INFO(arr_hash, pointer, key);

        if (key.type != HASH_KEY_IS_STRING) {
            DPRINTF("Invalid hash key data type: %u\n", key.type);
            goto cleanup;
        }

        for (i = 0; i < nhints; i++) {
            if (strcmp(key.name, hint[i].name) == 0)
                break;
        }

        if (i == nhints) {
            php_error_docref(NULL, E_WARNING,
                             "unknown key '%s'",
                             key.name);
            goto cleanup;
        }

        switch (hint[i].type) {
        case VIR_TYPED_PARAM_INT:
        case VIR_TYPED_PARAM_UINT:
        case VIR_TYPED_PARAM_LLONG:
        case VIR_TYPED_PARAM_ULLONG:
            if (Z_TYPE_P(data) != IS_LONG) {
                php_error_docref(NULL, E_WARNING,
                                 "invalied data type for key %s, expected long, got %u",
                                 key.name, Z_TYPE_P(data));
                goto cleanup;
            }
            switch (hint[i].type) {
            case VIR_TYPED_PARAM_INT:
                if (virTypedParamsAddInt(&params, &nparams, &maxparams, key.name, Z_LVAL_P(data)) < 0)
                    goto cleanup;
                break;
            case VIR_TYPED_PARAM_UINT:
                if (Z_LVAL_P(data) < 0) {
                    php_error_docref(NULL, E_WARNING,
                                     "invalid value for key %s: %ld", key.name, Z_LVAL_P(data));
                    goto cleanup;
                }
                if (virTypedParamsAddUInt(&params, &nparams, &maxparams, key.name, Z_LVAL_P(data)) < 0)
                    goto cleanup;
                break;
            case VIR_TYPED_PARAM_LLONG:
                if (virTypedParamsAddLLong(&params, &nparams, &maxparams, key.name, Z_LVAL_P(data)) < 0)
                    goto cleanup;
                break;
            case VIR_TYPED_PARAM_ULLONG:
                if (Z_LVAL_P(data) < 0) {
                    php_error_docref(NULL, E_WARNING,
                                     "invalid value for key %s: %ld", key.name, Z_LVAL_P(data));
                    goto cleanup;
                }
                if (virTypedParamsAddULLong(&params, &nparams, &maxparams, key.name, Z_LVAL_P(data)) < 0)
                    goto cleanup;
                break;
            }
            break;

        case VIR_TYPED_PARAM_DOUBLE:
            if (Z_TYPE_P(data) != IS_DOUBLE) {
                php_error_docref(NULL, E_WARNING,
                                 "invalied data type for key %s, expected double, got %u",
                                 key.name, Z_TYPE_P(data));
                goto cleanup;
            }
            if (virTypedParamsAddDouble(&params, &nparams, &maxparams, key.name, Z_DVAL_P(data)) < 0)
                goto cleanup;
            break;

        case VIR_TYPED_PARAM_BOOLEAN:
            if (Z_TYPE_P(data) != IS_TRUE && Z_TYPE_P(data) != IS_FALSE) {
                php_error_docref(NULL, E_WARNING,
                                 "invalied data type for key %s, expected true/false, got %u",
                                 key.name, Z_TYPE_P(data));
                goto cleanup;
            }

            if (virTypedParamsAddBoolean(&params, &nparams, &maxparams, key.name, Z_TYPE_P(data) == IS_TRUE) < 0)
                goto cleanup;
            break;

        case VIR_TYPED_PARAM_STRING:
            if (Z_TYPE_P(data) != IS_STRING) {
                php_error_docref(NULL, E_WARNING,
                                 "invalied data type for key %s, expected string, got %u",
                                 key.name, Z_TYPE_P(data));
                goto cleanup;
            }

            if (virTypedParamsAddString(&params, &nparams, &maxparams, key.name, Z_STRVAL_P(data)) < 0)
                goto cleanup;
            break;
        }
    }

    *paramsRet = params;
    params = NULL;

    *nparamsRet = nparams;
    nparams = 0;

    ret = 0;
 cleanup:
    virTypedParamsFree(params, nparams);
    return ret;
}
