#include <rte_ethdev.h>
#include <pcap.h>
#include <time.h>
extern "C"
{
    pcap_t *pcap_handle;
    pcap_dumper_t *pcap_dumper;
    struct pcap_pkthdr pcap_header;
    void init_args(void *to_next[8]){
        pcap_handle = pcap_open_dead(DLT_EN10MB, BUFSIZ);
        pcap_dumper = pcap_dump_open(pcap_handle, "output.pcap");
        if (pcap_dumper == NULL) return;
    }
    int plugin_0(struct rte_mbuf *pkt, char ****res, int rownum,int bucketnum,int bucketsize,int countersize, void *to_next[8])
    {
        pcap_header.ts.tv_sec = 0;
        pcap_header.ts.tv_usec = 0;
        pcap_header.caplen = pkt->pkt_len;
        pcap_header.len = pkt->pkt_len;
        char *pktbuf = rte_pktmbuf_mtod(pkt, char *);
        pcap_dump((u_char *)pcap_dumper, &pcap_header, (u_char *)pktbuf);
        return -1;
    }
    float evaluate_task_cpu(float c, float x, float v)
    {
        return x*v*c;
    }
    float evaluate_task_mem(float x, float v)
    {
        return x*v;
    }
    float g_cpu(float x, unsigned long long C)
    {
        return x*C;
    }
}