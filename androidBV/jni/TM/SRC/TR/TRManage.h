#ifndef TRMANAGE_H_
#define TRMANAGE_H_

#include "cpe.h"

#ifdef XNMP_ENV
#define evcpe_model_file        	"./testfiles/tr098_model.xml"
#define evcpe_data_file           	"./testfiles/tr098_data.xml"
#else
#define evcpe_model_file        	"./TM/SRC/TR/testfiles/tr098_model.xml"
#define evcpe_data_file           	"./TM/SRC/TR/testfiles/tr098_data.xml"
#endif

#define RST_ERR -1
#define RST_OK  0

#ifndef SUCCESS
/** @brief The null-pointer value. */
#define SUCCESS 0
#endif

#ifdef XNMP_ENV
#define FALSE -1
#define TRUE 0
#endif

#include "YKMsgQue.h"
extern YK_MSG_QUE_ST*  g_pstTMMsgQ;


int dispalyFlag;

int TMSetParamVersion(struct evcpe *cpe);
int TRProcessCommand(const char *cmd);

#endif /* TR069_INTF_LAYER_H_ */
