#include "threadpool.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <time.h>

void *thread_routine(void *arg)
{
	struct timespec abstime;
	int timeout;
	printf("thread 0x%x is starting\n", (int)pthread_self());
	threadpool_t *pool = (threadpool_t *)arg;
	while (1)
	{
		timeout = 0;
		condition_lock(&pool->ready);
		pool->idle++;
		// �ȴ������������������̳߳�����֪ͨ
		while (pool->first == NULL && !pool->quit)
		{
			printf("thread 0x%x is waiting\n", (int)pthread_self());
			//condition_wait(&pool->ready);
			clock_gettime(CLOCK_REALTIME, &abstime);
			abstime.tv_sec += 2;
			int status = condition_timedwait(&pool->ready, &abstime);
			if (status == ETIMEDOUT)//��ʱ
			{
				printf("thread 0x%x is wait timed out\n", (int)pthread_self());
				timeout = 1;
				break;
			}
		}

		// �ȴ������������ڹ���״̬
		pool->idle--;

		// �ȴ�������
		if (pool->first != NULL)
		{
			// �Ӷ�ͷȡ������
			task_t *t = pool->first;
			pool->first = t->next;
			// ִ��������Ҫһ����ʱ�䣬����Ҫ�Ƚ������Ա������߽���
			// �ܹ���������������������������߳��ܹ�����ȴ�����
			condition_unlock(&pool->ready);
			t->run(t->arg);
			free(t);
			condition_lock(&pool->ready);
		}
		// ����ȴ����̳߳�����֪ͨ, ������ִ�����
		if (pool->quit && pool->first == NULL)
		{
			pool->counter--;
			if (pool->counter == 0)
				condition_signal(&pool->ready);

			condition_unlock(&pool->ready);
			// ����ѭ��֮ǰҪ�ǵý���
			break;
		}

		if (timeout && pool->first == NULL)
		{
			pool->counter--;
			condition_unlock(&pool->ready);
			// ����ѭ��֮ǰҪ�ǵý���
			break;
		}
		condition_unlock(&pool->ready);
	}

	printf("thread 0x%x is exting\n", (int)pthread_self());
	return NULL;

}

// ��ʼ���̳߳�
void threadpool_init(threadpool_t *pool, int threads)
{
	// ���̳߳��еĸ����ֶγ�ʼ��
	condition_init(&pool->ready);
	pool->first = NULL;
	pool->last = NULL;
	pool->counter = 0;
	pool->idle = 0;
	pool->max_threads = threads;
	pool->quit = 0;
}

// ���̳߳����������
void threadpool_add_task(threadpool_t *pool, void *(*run)(void *arg), void *arg)
{
	// ����������
	task_t *newtask = (task_t *)malloc(sizeof(task_t));
	newtask->run = run;
	newtask->arg = arg;
	newtask->next = NULL;

	condition_lock(&pool->ready);
	// ��������ӵ�����
	if (pool->first == NULL)
		pool->first = newtask;
	else
		pool->last->next = newtask;
	pool->last = newtask;

	// ����еȴ��̣߳���������һ��
	if (pool->idle > 0)
		condition_signal(&pool->ready);
	else if (pool->counter < pool->max_threads)
	{
		// û�еȴ��̣߳����ҵ�ǰ�߳�������������߳������򴴽�һ�����߳�
		pthread_t tid;
		pthread_create(&tid, NULL, thread_routine, pool);
		pool->counter++;
	}
	condition_unlock(&pool->ready);
}

// �����̳߳�
void threadpool_destroy(threadpool_t *pool)
{
	if (pool->quit)
	{
		return;
	}
	condition_lock(&pool->ready);
	pool->quit = 1;
	if (pool->counter > 0)
	{
		if (pool->idle > 0)
			condition_broadcast(&pool->ready);

		// ����ִ������״̬�е��̣߳������յ��㲥
		// �̳߳���Ҫ�ȴ�ִ������״̬�е��߳�ȫ���˳�

		while (pool->counter > 0)
			condition_wait(&pool->ready);
	}
	condition_unlock(&pool->ready);
	condition_destroy(&pool->ready);


}
