long __mulsi3(long a, long b)
{
	long res = 0;
	unsigned long cnt = a;

	while (cnt) {
		if (cnt & 1) {
			res += b;
		}
		b <<= 1;
		cnt >>= 1;
	}

	return res;
}
long long __muldi3(long long a, long long b)
{
	long long res = 0;
	unsigned long long cnt = a;

	while (cnt) {
		if (cnt & 1) {
			res += b;
		}
		b <<= 1;
		cnt >>= 1;
	}

	return res;
}
