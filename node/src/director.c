#include "director.h"


struct flow_director * flow_list[MAX_FLOW_DIRECTOR_NUM];

struct flow_director *_register_flow(int port_id, MARKID markid, struct rte_flow *flow)
{
	if(flow == NULL){
		return NULL;
	}
	struct flow_director * fi= (struct flow_director *)malloc(sizeof(struct flow_director));;
	fi->port_id = port_id;
	fi->flow = flow;
	fi->markid = markid;
    int id = get_available_flow_id();
	flow_list[id]=fi;
	return fi;
}
void _unregister_flow_with_id(int id){
	free((void*)flow_list[id]->flow);//TODO
	free((void*)flow_list[id]);
	flow_list[id] = NULL;
}
void _unregister_flow(struct flow_director *flow_director){
	int id = find_flow_director(flow_director);
	if(id != -1){
		_unregister_flow_with_id(id);
	}
}

int get_available_flow_id(){
    int i = 0;
    for(;i<MAX_FLOW_DIRECTOR_NUM;i++){
        if(flow_list[i] == NULL){
            return i;
        }
    }
    return -1;
}
int find_flow_director(struct flow_director *flow_id){
	for (int i = 0; i < MAX_FLOW_DIRECTOR_NUM; i++)
	{
		if (flow_list[i] == flow_id)
		{
			return i;
		}
	}
	return -1;
}
int find_flow_director_with_objectid(int port_id,int object_id){
	for (int i = 0; i < MAX_FLOW_DIRECTOR_NUM; i++)
	{
		if (flow_list[i]->markid.id1.object_id == object_id&&flow_list[i]->port_id == port_id)
		{
			return i;
		}
	}
	return -1;
}

/**
 * create a flow rule that sends packets with matching src and dest ip
 * to selected queue.
 *
 * @param port_id
 *   The selected port.
 * @param markid
 *   The selected mark for every pluign runtime.
 * @param src_ip
 *   The src ip value to match the input packet.
 * @param src_mask
 *   The mask to apply to the src ip.
 * @param dest_ip
 *   The dest ip value to match the input packet.
 * @param dest_mask
 *   The mask to apply to the dest ip.
 * @param[out] error
 *   Perform verbose error reporting if not NULL.
 *
 * @return
 *   A flow if the rule could be created else return NULL.
 */

/* Function responsible for creating the flow rule. 8< */
struct flow_director*
generate_ipv4_flow_only(uint16_t port_id, MARKID markid, uint32_t priority,
		uint32_t src_ip, uint32_t src_mask,
		uint32_t dest_ip, uint32_t dest_mask,
		struct rte_flow_error *error)
{
	/* Declaring structs being used. 8< */
	struct rte_flow_attr attr;
	struct rte_flow_item pattern[MAX_PATTERN_NUM];
	struct rte_flow_action action[MAX_ACTION_NUM];
	struct rte_flow *flow = NULL;
	struct rte_flow_action_mark mark;
	struct rte_flow_item_ipv4 ip_spec;
	struct rte_flow_item_ipv4 ip_mask;
	/* >8 End of declaring structs being used. */
	int res;

	memset(pattern, 0, sizeof(pattern));
	memset(action, 0, sizeof(action));

	/* Set the rule attribute, only ingress packets will be checked. 8< */
	memset(&attr, 0, sizeof(struct rte_flow_attr));
	attr.ingress = 1;
	attr.priority = priority;
	/* >8 End of setting the rule attribute. */

	/*
	 * create the action sequence.
	 * one action only,  move packet to queue
	 */
	action[0].type = RTE_FLOW_ACTION_TYPE_MARK;
	mark.id = markid.id2;
	action[0].conf = &mark;
	action[1].type = RTE_FLOW_ACTION_TYPE_END;

	/*
	 * set the first level of the pattern (ETH).
	 * since in this example we just want to get the
	 * ipv4 we set this level to allow all.
	 */

	/* Set this level to allow all. 8< */
	pattern[0].type = RTE_FLOW_ITEM_TYPE_ETH;
	/* >8 End of setting the first level of the pattern. */

	/*
	 * setting the second level of the pattern (IP).
	 * in this example this is the level we care about
	 * so we set it according to the parameters.
	 */

	/* Setting the second level of the pattern. 8< */
	memset(&ip_spec, 0, sizeof(struct rte_flow_item_ipv4));
	memset(&ip_mask, 0, sizeof(struct rte_flow_item_ipv4));
	ip_spec.hdr.dst_addr = htonl(dest_ip);
	ip_mask.hdr.dst_addr = dest_mask;
	ip_spec.hdr.src_addr = htonl(src_ip);
	ip_mask.hdr.src_addr = src_mask;
	pattern[1].type = RTE_FLOW_ITEM_TYPE_IPV4;
	pattern[1].spec = &ip_spec;
	pattern[1].mask = &ip_mask;
	/* >8 End of setting the second level of the pattern. */

	/* The final level must be always type end. 8< */
	pattern[2].type = RTE_FLOW_ITEM_TYPE_END;
	/* >8 End of final level must be always type end. */

	/* Validate the rule and create it. 8< */
	res = rte_flow_validate(port_id, &attr, pattern, action, error);
	if (!res)
		flow = rte_flow_create(port_id, &attr, pattern, action, error);
		struct flow_director*fi = _register_flow(port_id,markid,flow);

	return fi;
}

void destroy_ipv4_flow(struct flow_director* flow_id)
{
	if(flow_id == NULL){
		return;
	}
	struct rte_flow_error error;
	int ret = rte_flow_destroy( flow_id->port_id,flow_id->flow,&error);
	if (ret) {
		printf("Flow can't be destroyed %d message: %s\n",
			error.type,
			error.message ? error.message : "(no stated reason)");
		rte_exit(EXIT_FAILURE, "error in destroying flow");
	}
	_unregister_flow(flow_id);
}
void destroy_ipv4_flow_with_id(int id){
	struct flow_director*flow_id = flow_list[id];
	destroy_ipv4_flow(flow_id);
}

void destroy_ipv4_flow_with_objectid(int port_id,int object_id){
	int id = find_flow_director_with_objectid(port_id,object_id);
	if(id != -1){
		destroy_ipv4_flow_with_id(id);
	}
}
int install_flow_director(unsigned int object_id,InterObject alpha){
	MARKID md;
	md.id1.object_id = object_id;
	
	struct flow_director *fi = generate_ipv4_flow_only(0,md,alpha.priority,
		alpha.in_ip,alpha.in_mask,alpha.out_ip,alpha.out_mask,
		NULL);
	if(fi == NULL){
		return -1;
	}
	fi = generate_ipv4_flow_only(0,md,alpha.priority,
		alpha.out_ip,alpha.out_mask,alpha.in_ip,alpha.in_mask,
		NULL);
	if(fi == NULL){
		return -1;
	}
	return 0;
}
int uninstall_flow_director(unsigned int object_id,InterObject alpha){
	MARKID md;
	md.id1.object_id = object_id;
	destroy_ipv4_flow_with_objectid(0,object_id);
	destroy_ipv4_flow_with_objectid(0,object_id);
	return 0;
}