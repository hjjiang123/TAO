#include <rte_ethdev.h>
extern "C"
{
    struct res_back{
        int epoch;
        void *handle;
        int run_actor_times_per_manage;
        uint64_t arrive_time;
        uint64_t retired_time;
        bool is_retired;
        float last_load;
        int last_load_type;
    };
    
    void init_args(void *to_next[8]){
    }
    int plugin_0(struct rte_mbuf *pkt, char ****res, int rownum,int bucketnum,int bucketsize,int countersize, void *to_next[8])
    {   
        int i=1;
        int n=1;
        while(i<320){
            n=n*i%11;
            i++;
        }
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