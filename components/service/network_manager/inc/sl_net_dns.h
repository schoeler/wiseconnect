/*******************************************************************************
* @file  sl_net_dns.h
* @brief 
*******************************************************************************
* # License
* <b>Copyright 2023 Silicon Laboratories Inc. www.silabs.com</b>
*******************************************************************************
*
* The licensor of this software is Silicon Laboratories Inc. Your use of this
* software is governed by the terms of Silicon Labs Master Software License
* Agreement (MSLA) available at
* www.silabs.com/about-us/legal/master-software-license-agreement. This
* software is distributed to you in Source Code format and is governed by the
* sections of the MSLA applicable to Source Code.
*
******************************************************************************/

#pragma once

#include "sl_ip_types.h"
#include "sl_status.h"
#include "sl_net_constants.h"

/** 
 * \addtogroup NET_INTERFACE_FUNCTIONS Network Interface
 * \ingroup SL_NET_FUNCTIONS
 * @{ */

/**
 * Resolve given host name to IP address.
 * @param host_name 			Host name which needs to be resolved.
 * @param timeout 				If value is greater than zero, caller would be blocked till timeout milliseconds to get the response.
 * 								If the values is equal's to zero, response would sent asynchronously using callbacks.
 * @param dns_resolution_ip 	Whether the host name needs to be resolved to IPV4 or IPV6.
 * @param ip_address 			IP address object to store resolved IP address
 * @return
 */
sl_status_t sl_dns_host_get_by_name(const char *host_name,
                                    const uint32_t timeout,
                                    const sl_net_dns_resolution_ip_type_t dns_resolution_ip,
                                    sl_ip_address_t *ip_address);

/** @} */
