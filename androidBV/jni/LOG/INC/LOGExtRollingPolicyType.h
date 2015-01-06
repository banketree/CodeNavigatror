/**
*@file   LOGExtRollingPolicyType.h
*@brief 扩展输出目标生成策略
*@author chensq
*@version 1.0
*@date 2012-4-23
*/

#ifndef ext_rollingpolicy_type_h
#define ext_rollingpolicy_type_h

#include <log4c/defs.h>
#include <log4c/rollingpolicy.h>

__LOG4C_BEGIN_DECLS



/**
 *@brief   初始化扩展rollingpolicy
 *@param   无
 *@return  0
 */
LOG4C_API int LOGExtRollingPolicyTypeInit(void);


__LOG4C_END_DECLS

#endif
