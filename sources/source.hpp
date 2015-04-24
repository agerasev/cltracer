#pragma once

static char *__loadSource(const char *fn, size_t *lenght)
{
	FILE *fp;
	size_t size;
	char *source = NULL;
	
	fp = fopen(fn,"r");
	if(!fp)
	{
			fprintf(stderr,"Failed to load shader: %s\n",fn);
			return NULL;
	}
	fseek(fp,0,SEEK_END);
	size = ftell(fp);
	fseek(fp,0,SEEK_SET);
	source = (char*)malloc(sizeof(char)*(size+1));
	fread(source,1,size,fp);
	source[size] = '\0';
	*lenght = size + 1;
	fclose(fp);
	return source;
}

static void __freeSource(char *source)
{
	free(source);
}
