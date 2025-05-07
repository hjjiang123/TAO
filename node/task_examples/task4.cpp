#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <rte_ethdev.h>
#include "public/kafka_p_c.h"
#define HASH_TABLE_SIZE 1024  // 哈希表大小
#define TIMEOUT 180           // 超时时间（3分钟）
#define MAX_PAYLOAD 1500      // 最大报文负载大小

extern "C"{
// TCP 报文结构
typedef struct Packet {
    uint32_t seq;              // 报文序列号
    uint32_t len;              // 报文负载长度
    char payload[MAX_PAYLOAD]; // 报文负载
    struct Packet *next;       // 下一个报文（用于乱序链表）
} Packet;

// TCP 流结构
typedef struct TcpStream {
    char src_ip[16];           // 源 IP 地址
    char dest_ip[16];          // 目标 IP 地址
    uint16_t src_port;         // 源端口
    uint16_t dest_port;        // 目标端口
    Packet *packet_list;       // 报文链表（按序列号排序）
    time_t last_update;        // 最后更新时间
    struct TcpStream *next;    // 用于存储在哈希表中的链表（解决哈希冲突）
} TcpStream;

// 哈希表，用于存储 TCP 流
TcpStream *hash_table[HASH_TABLE_SIZE];

// 计算哈希值
unsigned int hash_function(const char *src_ip, const char *dest_ip, uint16_t src_port, uint16_t dest_port) {
    unsigned int hash = 0;
    while (*src_ip) hash = (hash * 31) + *src_ip++;
    while (*dest_ip) hash = (hash * 31) + *dest_ip++;
    hash = (hash * 31) + src_port;
    hash = (hash * 31) + dest_port;
    return hash % HASH_TABLE_SIZE;
}

// 查找或创建 TCP 流
TcpStream *find_or_create_stream(const char *src_ip, const char *dest_ip, uint16_t src_port, uint16_t dest_port) {
    unsigned int hash = hash_function(src_ip, dest_ip, src_port, dest_port);
    TcpStream *stream = hash_table[hash];

    // 在哈希桶中查找对应的流
    while (stream) {
        if (strcmp(stream->src_ip, src_ip) == 0 &&
            strcmp(stream->dest_ip, dest_ip) == 0 &&
            stream->src_port == src_port &&
            stream->dest_port == dest_port) {
            return stream;
        }
        stream = stream->next;
    }

    // 未找到，创建新流
    stream = (TcpStream *)malloc(sizeof(TcpStream));
    strcpy(stream->src_ip, src_ip);
    strcpy(stream->dest_ip, dest_ip);
    stream->src_port = src_port;
    stream->dest_port = dest_port;
    stream->packet_list = NULL;
    stream->last_update = time(NULL);

    // 插入到哈希表中（头插法）
    stream->next = hash_table[hash];
    hash_table[hash] = stream;

    return stream;
}

// 按序列号插入报文到流中
void insert_packet_into_stream(TcpStream *stream, uint32_t seq, uint32_t len, const char *payload) {
    Packet *new_packet = (Packet *)malloc(sizeof(Packet));
    new_packet->seq = seq;
    new_packet->len = len;
    memcpy(new_packet->payload, payload, len);
    new_packet->next = NULL;

    Packet **current = &stream->packet_list;

    // 按序列号找到插入位置
    while (*current && (*current)->seq < seq) {
        current = &(*current)->next;
    }

    // 插入报文
    new_packet->next = *current;
    *current = new_packet;

    // 合并相邻报文
    while (new_packet->next && new_packet->seq + new_packet->len == new_packet->next->seq) {
        Packet *next_packet = new_packet->next;
        memcpy(new_packet->payload + new_packet->len, next_packet->payload, next_packet->len);
        new_packet->len += next_packet->len;
        new_packet->next = next_packet->next;
        free(next_packet);
    }

    // 更新最后更新时间
    stream->last_update = time(NULL);
}

// 清理超时流
void cleanup_expired_streams() {
    time_t now = time(NULL);

    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        TcpStream **current = &hash_table[i];

        while (*current) {
            TcpStream *stream = *current;

            // 检查是否超时
            if (now - stream->last_update > TIMEOUT) {
                // 清理流
                Packet *packet = stream->packet_list;
                while (packet) {
                    Packet *next_packet = packet->next;
                    free(packet);
                    packet = next_packet;
                }
                *current = stream->next;
                free(stream);
            } else {
                current = &stream->next;
            }
        }
    }
}

// 打印流内容
void print_streams() {
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        TcpStream *stream = hash_table[i];

        while (stream) {
            printf("Stream: %s:%d -> %s:%d\n", stream->src_ip, stream->src_port, stream->dest_ip, stream->dest_port);

            Packet *packet = stream->packet_list;
            while (packet) {
                printf("  Packet: seq=%u, len=%u\n", packet->seq, packet->len);
                packet = packet->next;
            }
            stream = stream->next;
        }
    }
}

}
// 示例测试
int main() {
    // 示例报文数据
    insert_packet_into_stream(find_or_create_stream("192.168.1.1", "192.168.1.2", 12345, 80), 0, 5, "Hello");
    insert_packet_into_stream(find_or_create_stream("192.168.1.1", "192.168.1.2", 12345, 80), 5, 5, "World");
    insert_packet_into_stream(find_or_create_stream("192.168.1.1", "192.168.1.2", 12345, 80), 10, 5, "Again");

    // 打印流
    print_streams();

    // 模拟超时清理
    sleep(181);
    cleanup_expired_streams();

    // 再次打印流
    print_streams();

    return 0;
}