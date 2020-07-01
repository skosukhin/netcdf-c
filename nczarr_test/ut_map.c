/*
 *	Copyright 2018, University Corporation for Atmospheric Research
 *      See netcdf/COPYRIGHT file for copying and redistribution conditions.
 */

#include "ut_includes.h"

#undef DEBUG

#if 0
#define FILE1 "testmapnc4.ncz"
#define URL1 "file://" FILE1 "#mode=zarr"
#endif

#define META1 "/meta1"
#define META2 "/meta2"
#define DATA1 "/data1"
#define DATA1LEN 25

static const char* metadata1 = "{\n\"foo\": 42,\n\"bar\": \"apples\",\n\"baz\": [1, 2, 3, 4]}";

static const char* metadata2 = "{\n\"foo\": 42,\n\"bar\": \"apples\",\n\"baz\": [1, 2, 3, 4],\n\"extra\": 137}";

static char* url = NULL;
static NCZM_IMPL impl = NCZM_UNDEF;

/* Forward */
static int simplecreate(void);
static int simpledelete(void);
static int writemeta(void);
static int writemeta2(void);
static int readmeta(void);
static int writedata(void);
static int readdata(void);
static int search(void);

struct Test tests[] = {
{"create",simplecreate},
{"delete",simpledelete},
{"writemeta", writemeta},
{"writemeta2", writemeta2},
{"readmeta", readmeta},
{"writedata", writedata},
{"readdata", readdata},
{"search", search},
{NULL,NULL}
};

int
main(int argc, char** argv)
{
    int stat = NC_NOERR;

    if((stat = ut_init(argc, argv, &options))) goto done;
    if(options.file == NULL && options.output != NULL) options.file = strdup(options.output);
    if(options.output == NULL && options.file != NULL)options.output = strdup(options.file);
    impl = kind2impl(options.kind);
    url = makeurl(options.file,impl);

    if((stat = runtests((const char**)options.cmds,tests))) goto done;
    
done:
    if(stat) usage(stat);
    return 0;
}

/* Do a simple create */
static int
simplecreate(void)
{
    int stat = NC_NOERR;
    NCZMAP* map = NULL;
    char* path = NULL;

    if((stat = nczmap_create(impl,url,0,0,NULL,&map)))
	goto done;

    if((stat=nczm_concat(NULL,NCZMETAROOT,&path)))
	goto done;
    if((stat = nczmap_defineobj(map, path)))
	goto done;

    /* Do not delete so we can look at it with ncdump */
    if((stat = nczmap_close(map,0)))
	goto done;
done:
    nullfree(path);
    return THROW(stat);
}

/* Do a simple delete of previously created file */
static int
simpledelete(void)
{
    int stat = NC_NOERR;
    NCZMAP* map = NULL;

    if((stat = nczmap_open(impl,url,0,0,NULL,&map)))
	goto done;
    if((stat = nczmap_close(map,1)))
	goto done;

done:
    return THROW(stat);
}

static int
writemeta(void)
{
    int stat = NC_NOERR;
    NCZMAP* map = NULL;
    char* path = NULL;

    if((stat = nczmap_create(impl,url,0,0,NULL,&map)))
	goto done;

    if((stat=nczm_concat(NULL,NCZMETAROOT,&path)))
	goto done;
    if((stat = nczmap_defineobj(map, path)))
	goto done;
    free(path); path = NULL;

    if((stat=nczm_concat(META1,ZARRAY,&path)))
	goto done;
    if((stat = nczmap_defineobj(map, path)))
	goto done;
    if((stat = nczmap_write(map, path, 0, strlen(metadata1), metadata1)))
	goto done;
    free(path); path = NULL;

done:
    /* Do not delete so we can look at it with ncdump */
    (void)nczmap_close(map,0);
    nullfree(path);
    return THROW(stat);
}

static int
writemeta2(void)
{
    int stat = NC_NOERR;
    NCZMAP* map = NULL;
    char* path = NULL;

    if((stat = nczmap_open(impl,url,NC_WRITE,0,NULL,&map)))
	goto done;

    if((stat=nczm_concat(META2,NCZVAR,&path)))
	goto done;
    if((stat = nczmap_defineobj(map,path)))
	goto done;
    if((stat = nczmap_write(map, path, 0, strlen(metadata2), metadata2)))
	goto done;

done:
    /* Do not delete so we can look at it with ncdump */
    (void)nczmap_close(map,0);
    nullfree(path);
    return THROW(stat);
}

static int
readmeta(void)
{
    int stat = NC_NOERR;
    NCZMAP* map = NULL;
    char* path = NULL;
    size64_t olen;
    char* content = NULL;

    if((stat = nczmap_open(impl,url,0,0,NULL,&map)))
	goto done;

    if((stat=nczm_concat(META1,ZARRAY,&path)))
	goto done;

    /* Get length */
    if((stat = nczmap_len(map, path, &olen)))
	goto done;

    /* Allocate the space for reading the metadata (might be overkill) */
    if((content = malloc(olen+1)) == NULL)
	{stat = NC_ENOMEM; goto done;}

    if((stat = nczmap_read(map, path, 0, olen, content)))
	goto done;

    /* nul terminate */
    content[olen] = '\0';

    printf("%s: |%s|\n",path,content);

done:
    (void)nczmap_close(map,0);
    nullfree(content);
    nullfree(path);
    return THROW(stat);
}

static int
writedata(void)
{
    int stat = NC_NOERR;
    NCZMAP* map = NULL;
    char* path = NULL;
    int data1[DATA1LEN];
    int i;
    size64_t totallen;
    char* data1p = (char*)&data1[0]; /* byte level version of data1 */

    /* Create the data */
    for(i=0;i<DATA1LEN;i++) data1[i] = i;
    totallen = sizeof(int)*DATA1LEN;

    if((stat = nczmap_open(impl,url,NC_WRITE,0,NULL,&map)))
	goto done;

    /* ensure object */
    if((stat=nczm_concat(DATA1,"0",&path)))
	goto done;

    if((stat = nczmap_defineobj(map,path)))
	goto done;

    /* Write in 3 slices */
    for(i=0;i<3;i++) {
        size64_t start, count, third, last;
	third = (totallen+2) / 3; /* round up */
        start = i * third;
	last = start + third;
	if(last > totallen) 
	    last = totallen;
	count = last - start;
        
	if((stat = nczmap_write(map, path, start, count, &data1p[start])))
	     goto done;
    }

done:
    /* Do not delete so we can look at it with ncdump */
    (void)nczmap_close(map,0);
    nullfree(path);
    return THROW(stat);
}

static int
readdata(void)
{
    int stat = NC_NOERR;
    NCZMAP* map = NULL;
    char* path = NULL;
    int data1[DATA1LEN];
    int i;
    size64_t chunklen, totallen;
    char* data1p = NULL; /* byte level pointer into data1 */

    if((stat = nczmap_open(impl,url,0,0,NULL,&map)))
	goto done;

    /* ensure object */
    if((stat=nczm_concat(DATA1,"0",&path)))
	goto done;

    if((stat = nczmap_exists(map,path)))
	goto done;

    /* Read chunks in size sizeof(int)*n, where is rndup(DATA1LEN/3) */
    chunklen = sizeof(int) * ((DATA1LEN+2)/3);
    data1p = (char*)&data1[0];
    totallen = sizeof(int)*DATA1LEN;

    /* Read in 3 chunks */
    memset(data1,0,sizeof(data1));
    for(i=0;i<3;i++) {
        size64_t start, count, last;
        start = i * chunklen;
	last = start + chunklen;
	if(last > totallen) 
	    last = totallen;
	count = last - start;
	if((stat = nczmap_read(map, path, start, count, &data1p[start])))
	     goto done;
    }

    /* Validate */
    for(i=0;i<DATA1LEN;i++) {
	if(data1[i] != i) {
	    fprintf(stderr,"data mismatch: is: %d should be: %d\n",data1[i],i);
	    stat = NC_EINVAL;
	    goto done;
	}
    }

done:
    /* Do not delete so we can look at it with ncdump */
    (void)nczmap_close(map,0);
    nullfree(path);
    return THROW(stat);
}

static int
searchR(NCZMAP* map, int depth, const char* prefix, NClist* objects)
{
    int i,stat = NC_NOERR;
    NClist* matches = nclistnew();
    
    /* add this prefix to object list */
    nclistpush(objects,strdup(prefix));
    
    /* get next level object keys **below** the prefix */
    if((stat = nczmap_search(map, prefix, matches)))
	goto done;
    for(i=0;i<nclistlength(matches);i++) {
	const char* key = nclistget(matches,i);
        if((stat = searchR(map,depth+1,key,objects))) goto done;
	if(stat != NC_NOERR)
	    goto done;
    }

done:
    nclistfreeall(matches);
    return stat;
}

static int
search(void)
{
    int i,stat = NC_NOERR;
    NCZMAP* map = NULL;
    NClist* objects = nclistnew();

    if((stat = nczmap_open(impl,url,0,0,NULL,&map)))
	goto done;

    /* Do a recursive search on root to get all object keys */
    if((stat=searchR(map,0,"/",objects)))
	goto done;
    /* Print out the list */
    for(i=0;i<nclistlength(objects);i++) {
	const char* key = nclistget(objects,i);
	printf("[%d] %s\n",i,key);
    }

done:
    /* Do not delete so later tests can use it */
    (void)nczmap_close(map,0);
    nclistfreeall(objects);
    return THROW(stat);
}
