#ifndef _THREAD_POOL_H_
#define _THREAD_POOL_H_

#include "condition.h"

// ����ṹ��,���������������̳߳��е��߳���ִ��
typedef struct task
{
	void *(*run)(void *arg);	// ����ص�����
	void *arg;					// �ص���������
	struct task *next;
} task_t;

// �̳߳ؽṹ��
typedef struct threadpool
{
	condition_t ready;		//����׼�����������̳߳�����֪ͨ
	task_t *first;			//�������ͷָ��
	task_t *last;			//�������βָ��
	int counter;			//�̳߳��е�ǰ�߳���
	int idle;				//�̳߳��е�ǰ���ڵȴ�������߳���
	int max_threads;		//�̳߳������������߳���
	int quit;				//�����̳߳ص�ʱ����1
} threadpool_t;

// ��ʼ���̳߳�
void threadpool_init(threadpool_t *pool, int threads);
// ���̳߳����������
void threadpool_add_task(threadpool_t *pool, void *(*run)(void *arg), void *arg);
// �����̳߳�
void threadpool_destroy(threadpool_t *pool);

#endif /* _THREAD_POOL_H_ */
