#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include "param.h"
#include "scheduler.h"
#include "lib/cJSON.h"

size_t write_callback_measure_count_beta(void *ptr, size_t size, size_t nmemb, void *userdata){
	cJSON *json = cJSON_Parse((char*)ptr);
    if (json == NULL) {
        printf("Failed to parse JSON: %s\n", cJSON_GetErrorPtr());
        return 0;
    }

    // 获取键值对
    cJSON *v1 = cJSON_GetObjectItem(json, "119839591101537");
	if (v1 == NULL || v1->type != cJSON_Array) {
        printf("Failed to get array object.\n");
        cJSON_Delete(json);
        return 0;
    }

	cJSON *v2 = cJSON_GetArrayItem(v1, 0);
    if (v2 == NULL || v2->type != cJSON_Object) {
        printf("Failed to get array item.\n");
        cJSON_Delete(json);
        return 0;
    }
	cJSON *v3 = cJSON_GetObjectItem(v2, "packet_count");
	
	float count_beta = (float)v3->valuedouble;
	*(float*)userdata=count_beta;
	cJSON_Delete(json);
	return sizeof(float);
}
int post_command_with_result(const char *command,const char *args, void *result)
{
	
	CURL *curl;
	CURLcode res;
	// float *chunk = (float *)result;
	struct curl_slist *http_header = NULL;
 
	curl = curl_easy_init();
	if (!curl)
	{
		fprintf(stderr,"curl init failed\n");
		return -1;
	}
	char *url = (char *)malloc(strlen(POSTURL) + strlen(command) + 1);
	strcpy(url, POSTURL);
	strcat(url, command);
	curl_easy_setopt(curl,CURLOPT_URL,url); //url地址
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0); //不检查ssl，可访问https
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);  //不检查ssl，可访问https
	
	curl_easy_setopt(curl,CURLOPT_POSTFIELDS,args); //post参数
	curl_easy_setopt(curl,CURLOPT_POST,1); //设置问非0表示本次操作为post
	// curl_easy_setopt(curl,CURLOPT_VERBOSE,1); //打印调试信息

	http_header = curl_slist_append(NULL, "Content-Type:application/json;charset=UTF-8");
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, http_header);

	curl_easy_setopt(curl,CURLOPT_FOLLOWLOCATION,1); //设置为非0
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);//接收数据时超时设置，如果10秒内数据未接收完，直接退出
 
	// 设置回调函数，用于接收响应数据
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback_measure_count_beta);

	// 设置回调函数的用户数据
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, result);
	res = curl_easy_perform(curl);
 
	if (res != CURLE_OK)
	{
		switch(res)
		{
			case CURLE_UNSUPPORTED_PROTOCOL:
				fprintf(stderr,"不支持的协议,由URL的头部指定\n");
			case CURLE_COULDNT_CONNECT:
				fprintf(stderr,"不能连接到remote主机或者代理\n");
			case CURLE_HTTP_RETURNED_ERROR:
				fprintf(stderr,"http返回错误\n");
			case CURLE_READ_ERROR:
				fprintf(stderr,"读本地文件错误\n");
			default:
				fprintf(stderr,"返回值:%d\n",res);
		}
		return -1;
	}
	curl_easy_cleanup(curl);
    printf("\n");
    return 0;
}

int post_command_without_result(const char *command,const char *args)
{
	
	CURL *curl;
	CURLcode res;
	struct curl_slist *http_header = NULL;
 
	curl = curl_easy_init();
	if (!curl)
	{
		fprintf(stderr,"curl init failed\n");
		return -1;
	}
	char *url = (char *)malloc(strlen(POSTURL) + strlen(command) + 1);
	strcpy(url, POSTURL);
	strcat(url, command);
	curl_easy_setopt(curl,CURLOPT_URL,url); //url地址
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0); //不检查ssl，可访问https
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);  //不检查ssl，可访问https
	
	curl_easy_setopt(curl,CURLOPT_POSTFIELDS,args); //post参数
	curl_easy_setopt(curl,CURLOPT_POST,1); //设置问非0表示本次操作为post

	http_header = curl_slist_append(NULL, "Content-Type:application/json;charset=UTF-8");
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, http_header);

	curl_easy_setopt(curl,CURLOPT_FOLLOWLOCATION,1); //设置为非0
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);//接收数据时超时设置，如果10秒内数据未接收完，直接退出
 
	res = curl_easy_perform(curl);
 
	if (res != CURLE_OK)
	{
		switch(res)
		{
			case CURLE_UNSUPPORTED_PROTOCOL:
				fprintf(stderr,"不支持的协议,由URL的头部指定\n");
			case CURLE_COULDNT_CONNECT:
				fprintf(stderr,"不能连接到remote主机或者代理\n");
			case CURLE_HTTP_RETURNED_ERROR:
				fprintf(stderr,"http返回错误\n");
			case CURLE_READ_ERROR:
				fprintf(stderr,"读本地文件错误\n");
			default:
				fprintf(stderr,"返回值:%d\n",res);
		}
		return -1;
	}
	curl_easy_cleanup(curl);
    printf("\n");
    return 0;
}

// int main(int argc, char *argv[])
// {
// 	char * command = "stats/groupentry/add";
// 	char * postfield = "{\"dpid\": \"1\",\"group_id\": 1,\"type\": \"SELECT\",\"buckets\": [{\"weight\": 1,\"actions\": [{\"type\": \"OUTPUT\", \"port\": 2}]},{\"weight\": 1,\"actions\": [{\"type\": \"OUTPUT\", \"port\": 3}]},{\"weight\": 2,\"actions\": [{\"type\": \"OUTPUT\", \"port\": 4}]},{\"weight\": 2,\"actions\": [{\"type\": \"OUTPUT\", \"port\": 1}]}]}";
// 	post_command(command, postfield);
// 	return 0;
// }