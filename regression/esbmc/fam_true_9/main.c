typedef struct {
  int *a[2];
  int *b[2];
  int *free[]; // int *free[1]
} FAM;

int array1[2];
int array2[2];
int end;
int other;

FAM fam = {
	{
		&array1[0]
	}
	,
	{
		&array2[0]
	}
	,
	{
		&other,
		&end
	}
};

void FamInit()
{
	unsigned i;
	int **p = fam.a;

	p[0] = &array1[0];
        p[1] = &array1[1];

	p = &fam.a[2]; // array2
	p[0] = &array2[0];
	p[1] = &array2[1];
}


#include <assert.h>
int main() {
  FamInit();
  FAM *dst = (FAM*) malloc(sizeof(int*) * 6);
  memcpy(dst->free, &fam.free, sizeof(int*) * 6);
  int **pra = dst->a;
  //assert(pra[0] == &array1[0]);
  //assert(pra[1] == &array1[1]);
  //assert(pra[2] == &array2[0]);
  //assert(pra[3] == &array2[1]);
  assert(pra[4] == &other);
  assert(pra[5] == &end);
}
