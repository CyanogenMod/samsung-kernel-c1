/*===========================================================================

                        SiI9234_DRIVER.H


DESCRIPTION
  This file explains the SiI9234 initialization and call the virtual main function.
 

 Copyright (c) 2002-2009, Silicon Image, Inc.  All rights reserved.
  No part of this work may be reproduced, modified, distributed, transmitted,
 transcribed, or translated into any language or computer format, in any form
or by any means without written permission of: Silicon Image, Inc.,
1060 East Arques Avenue, Sunnyvale, California 94085
===========================================================================*/


/*===========================================================================

                      EDIT HISTORY FOR FILE

when              who                         what, where, why
--------        ---                        ----------------------------------------------------------
2010/11/06    Daniel Lee(Philju)      Initial version of file, SIMG Korea
===========================================================================*/

/*===========================================================================
                     INCLUDE FILES FOR MODULE
===========================================================================*/


/*===========================================================================
                   FUNCTION DEFINITIONS
===========================================================================*/


void sii9234_interrupt_event(void);
bool sii9234_init(void);

void sii9234_unmaks_interrupt(void);

//MHL IOCTL INTERFACE
#define MHL_READ_RCP_DATA 0x1

