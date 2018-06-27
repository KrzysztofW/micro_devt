#include <stdio.h>
#include "array.h"

void array_print(int *array, unsigned int size)
{
	unsigned int i;

	for (i = 0; i < size; i++)
		printf(" %d", array[i]);
	puts("");
}

void array_shell_sort(int *array, unsigned int size)
{
	unsigned int g;

	for (g = size / 2; g > 0; g /= 2) {
		unsigned int i;

		for (i = g; i < size; i++) {
			int tmp = array[i];
			unsigned int j;

			for (j = i; j >= g && array[j - g] > tmp; j -= g)
				array[j] = array[j - g];
			array[j] = tmp;
		}
	}
}

int array_get_median(int *array, unsigned int size)
{
	unsigned int n = size / 2;

	array_shell_sort(array, size);
	if (size & 0x1)
		return array[n];
	return (array[n] + array[n - 1]) / 2;
}

int array_get_average(int *array, unsigned int size)
{
	unsigned int i;
	int res = 0;

	for (i = 0; i < size; i++)
		res += array[i];
	return res / size;
}


#if TEST
static int array_shell_sort_check(int *array, unsigned int size)
{
	unsigned int i;

	array_shell_sort(array, size);
	for (i = 1; i < size; i++) {
		if (array[i] < array[i - 1]) {
			fprintf(stderr, "failed sorting array: ");
			array_print(array, size);
			return -1;
		}
	}
	return 0;
}

static int array_shell_sort_tests(void)
{
	int a[] = { 4, -1, 55, 23, 8, -5, 4, 99, 90, 90 };
	int b = 1;
	int c[] = { 2 };
	int *d = NULL;
	int size_a = sizeof(a) / sizeof(int);
	int size_b = sizeof(b) / sizeof(int);
	int size_c = sizeof(c) / sizeof(int);

	if (array_shell_sort_check(a, size_a) < 0)
		return -1;
	if (array_shell_sort_check(&b, size_b) < 0)
		return -1;
	if (array_shell_sort_check(c, size_c) < 0)
		return -1;
	if (array_shell_sort_check(d, 0) < 0)
		return -1;

	return 0;
}

static int array_median_tests(void)
{
	int a[] = { 4, -1, 55, 23, 8, -5, 4, 99, 90, 90 };
	int b[] = { 1 };
	int c[] = { 2, 3};
	int d[] = { -1, 2, 3};

	if (array_get_median(a, sizeof(a) / sizeof(int)) != 15)
		goto error;
	if (array_get_median(b, sizeof(b) / sizeof(int)) != 1)
		goto error;
	if (array_get_median(c, sizeof(c) / sizeof(int)) != 2)
		goto error;
	if (array_get_median(d, sizeof(d) / sizeof(int)) != 2)
		goto error;

	return 0;
 error:
	fprintf(stderr, "%s: failed\n", __func__);
	return -1;
}

static int array_average_tests(void)
{
	int a[] = { 4, -1, 55, 23, 8, -5, 4, 99, 90, 90 };
	int b[] = { 1 };
	int c[] = { 2, 4};
	int d[] = { -1, 2, 3};

	if (array_get_average(a, sizeof(a) / sizeof(int)) != 36)
		goto error;
	if (array_get_average(b, sizeof(b) / sizeof(int)) != 1)
		goto error;
	if (array_get_average(c, sizeof(c) / sizeof(int)) != 3)
		goto error;
	if (array_get_average(d, sizeof(d) / sizeof(int)) != 1)
		goto error;

	return 0;
 error:
	fprintf(stderr, "%s: failed\n", __func__);
	return -1;
}

int array_tests(void)
{
	if (array_shell_sort_tests() < 0)
		return -1;
	if (array_median_tests() < 0)
		return -1;
	if (array_average_tests() < 0)
		return -1;
	return 0;
}
#endif

