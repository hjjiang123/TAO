#include "thread.h"
#include "server.h"
#include "scheduler.h"
#include <rte_ethdev.h>
#include <pthread.h>
#define RX_DESC_DEFAULT 512

void print_device_info(uint16_t port_id) {
    struct rte_eth_dev_info dev_info;
    if (rte_eth_dev_info_get(port_id, &dev_info) != 0) {
        printf("Failed to get device info for port %u\n", port_id);
        return;
    }

    printf("Device info for port %u:\n", port_id);
    printf("Driver name: %s\n", dev_info.driver_name);
    printf("Max RX queues: %u\n", dev_info.max_rx_queues);
    printf("Max TX queues: %u\n", dev_info.max_tx_queues);
    printf("RX offload capabilities: 0x%" PRIx64 "\n", dev_info.rx_offload_capa);
    printf("TX offload capabilities: 0x%" PRIx64 "\n", dev_info.tx_offload_capa);

    // 检查支持的统计字段
    struct rte_eth_stats stats;
    if (rte_eth_stats_get(port_id, &stats) == 0) {
        printf("Supported statistics fields:\n");
        printf("  ipackets: %s\n", stats.ipackets ? "yes" : "no");
        printf("  opackets: %s\n", stats.opackets ? "yes" : "no");
        printf("  ibytes: %s\n", stats.ibytes ? "yes" : "no");
        printf("  obytes: %s\n", stats.obytes ? "yes" : "no");
        printf("  imissed: %s\n", stats.imissed ? "yes" : "no");
        printf("  ierrors: %s\n", stats.ierrors ? "yes" : "no");
        printf("  oerrors: %s\n", stats.oerrors ? "yes" : "no");
        printf("  rx_nombuf: %s\n", stats.rx_nombuf ? "yes" : "no");
    } else {
        printf("Failed to get statistics for port %u\n", port_id);
    }
}

static const struct rte_eth_conf port_conf_default = {
	.rxmode = {
		.mq_mode = RTE_ETH_MQ_RX_RSS,
		.max_lro_pkt_size = RTE_ETHER_MAX_LEN,
	},
	.txmode = {
		.mq_mode = RTE_ETH_MQ_TX_NONE,
	},
	.rx_adv_conf = {
		.rss_conf = {
			.rss_hf = RTE_ETH_RSS_IPV4,
			.algorithm = RTE_ETH_HASH_FUNCTION_DEFAULT
		}
	},
};

// 初始化网卡端口
int init_port(unsigned int port_id)
{
	int ret;
	
	// 判断该网卡端口是否存在
	ret = rte_eth_dev_is_valid_port(port_id);
	if (ret == 0) {
		rte_exit(EXIT_FAILURE, "Invalid port_id %d\n", port_id);
	}

	// 设置网卡端口的配置
	ret = rte_eth_dev_configure(port_id, THREAD_INPUT_THREAD_NUM, 0, &port_conf_default);
	if (ret < 0) {
		rte_exit(EXIT_FAILURE, "Cannot configure device: err=%d, port=%d\n", ret, port_id);
		return ret;
	}
	// 创建内存池
	struct rte_mempool *mbuf_pool = rte_pktmbuf_pool_create("mempool", NUM_MBUFS_DEFAULT, MBUF_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());
	if (mbuf_pool == NULL) {
		rte_exit(EXIT_FAILURE, "Cannot create mbuf pool\n");
	}
	// 启动队列
	for(int i = 0; i < THREAD_INPUT_THREAD_NUM; i++){
		ret = rte_eth_rx_queue_setup(port_id, i, RX_DESC_DEFAULT, rte_eth_dev_socket_id(port_id), NULL, mbuf_pool);
		if (ret < 0) {
			rte_exit(EXIT_FAILURE, "rte_eth_rx_queue_setup:err=%d, port=%d\n", ret, port_id);
			return ret;
		}
	}

	// 开启混杂模式
	rte_eth_promiscuous_enable(port_id);

	// 启动网卡端口
	ret = rte_eth_dev_start(port_id);

	return 0;
}
int main(int argc, char **argv){
	int ret = rte_eal_init(argc, argv);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "Error with EAL initialization\n");
	
	// 获取绑定的设备数量
    uint16_t nb_ports = rte_eth_dev_count_avail();
    printf("Number of available Ethernet devices: %u\n", nb_ports);

    // 遍历并打印每个绑定的网卡信息
    for (int i = 0; i < nb_ports; i++) {
        struct rte_eth_dev_info dev_info;
        rte_eth_dev_info_get(i, &dev_info);

        printf("Port %u:\n", i);
        printf("  - Driver name: %s\n", dev_info.driver_name);
        printf("  - Max RX queues: %u\n", dev_info.max_rx_queues);
        printf("  - Max TX queues: %u\n", dev_info.max_tx_queues);
    }
	// print_device_info(0);
	init_port(DPDK_PORT);
	if (ret < 0) {
		rte_exit(EXIT_FAILURE, "Cannot init port %d\n", 0);
	}
	
    /* Pipeline */
	int status = subtask_init();
	if (status) {
		printf("Error: Subtask initialization failed (%d)\n", status);
		return status;
	}

	/* Thread */
	status = thread_init();
	if (status) {
		printf("Error: Thread initialization failed (%d)\n", status);
		return status;
	}
	init_task_id_system();
	init_actor_id_system();
	init_objects();
	// initial_filter();
	/* Scheduler */

	uint32_t lcoreid = rte_lcore_id();
	uint32_t i;
	RTE_LCORE_FOREACH_WORKER(i) {
		if(i == lcoreid){
			continue;
		}else if (i==1){
			rte_eal_remote_launch(init_server, NULL, i);
		}else if(i>=2&&i<2+THREAD_INPUT_THREAD_NUM){
			int queue_id = i-2;
			rte_eal_remote_launch(thread_input, &queue_id, i);
		}
		else if(i==39){
			rte_eal_remote_launch(sche_InitialScheduling, NULL, i);
		}else if(i==38){
			rte_eal_remote_launch(sche_rescheduling_2, NULL, i);
		}else if(i>=2+THREAD_INPUT_THREAD_NUM&&i<MAX_CORE_NUM){
			rte_eal_remote_launch(thread_main, NULL, i);
		}
		
	}
	// status = runserver();
	if (status) {
		printf("Error: Thread runserver failed (%d)\n", status);
		return status;
	}
	rte_eal_mp_wait_lcore();
}