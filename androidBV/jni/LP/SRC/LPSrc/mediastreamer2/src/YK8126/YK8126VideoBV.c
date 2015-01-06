

#ifdef _YK_XT8126_BV_

extern int VideoRecIsStart;

void YK8126VideoBVResWait(void)
{
	printf("[*****clear video resource ...*****]\n");
	while(VideoRecIsStart)
	{
		usleep(10*1000);
	}
	printf("[*****clear video resource ok !*****]\n");
}

#endif
