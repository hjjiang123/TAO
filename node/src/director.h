#ifndef DIRECTOR_H
#define DIRECTOR_H
/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2017 Mellanox Technologies, Ltd
 */
#include <rte_flow.h>
#include <stdlib.h>
#include "param.h"

#define MAX_PATTERN_NUM		3
#define MAX_ACTION_NUM		2

struct flow_director {
	int port_id;
	MARKID markid;
	struct rte_flow *flow;
};


struct flowfilter_entry{
	unsigned int src_ip;
	unsigned int src_mask;
	unsigned int dst_ip;
	unsigned int dst_mask;
	int src_port_num;
	unsigned short src_port;
	int dst_port_num;
	unsigned short dst_port;
};
int get_available_flow_id();
int find_flow_director(struct flow_director *flow_id);

struct flow_director*
generate_ipv4_flow_only(uint16_t port_id, MARKID markid, uint32_t priority,
		uint32_t src_ip, uint32_t src_mask,
		uint32_t dest_ip, uint32_t dest_mask,
		struct rte_flow_error *error);
int find_flow_director_with_objectid(int port_id,int object_id);
void destroy_ipv4_flow_with_id(int id);
void destroy_ipv4_flow_with_objectid(int port_id,int mark_id);

int install_flow_director(unsigned int object_id,InterObject alpha);
int uninstall_flow_director(unsigned int object_id,InterObject alpha);
#endif