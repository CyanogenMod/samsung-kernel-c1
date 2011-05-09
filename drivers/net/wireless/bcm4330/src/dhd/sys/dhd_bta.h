/*
 * BT-AMP support routines
 *
 * Copyright (C) 2010, Broadcom Corporation
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 * $Id: dhd_bta.h,v 1.1.14.1.30.1 2009/06/18 01:10:20 Exp $
 */
#ifndef __dhd_bta_h__
#define __dhd_bta_h__

struct dhd_pub;

extern int dhd_bta_docmd(struct dhd_pub *pub, void *cmd_buf, uint cmd_len);

extern void dhd_bta_doevt(struct dhd_pub *pub, void *data_buf, uint data_len);

extern int dhd_bta_tx_hcidata(struct dhd_pub *pub, void *data_buf, uint data_len);
extern void dhd_bta_tx_hcidata_complete(struct dhd_pub *dhdp, void *txp, bool success);

struct amp_hci_event;
struct amp_hci_ACL_data;

extern void dhd_bta_hcidump_evt(struct dhd_pub *pub, struct amp_hci_event *event);
extern void dhd_bta_hcidump_ACL_data(struct dhd_pub *pub, struct amp_hci_ACL_data *ACL_data,
                                     bool tx);
#endif /* __dhd_bta_h__ */
