#include <Enesim.h>
#include <string.h>
#include <stdio.h>

typedef struct _Abstract01
{
	Enesim_Object_Instance parent;
} Abstract01;

typedef struct _Abstract01_Class
{
	Enesim_Object_Class parent;
	const char *(*name_get)(Abstract01 *o);
} Abstract01_Class;

ENESIM_OBJECT_ABSTRACT_BOILERPLATE(enesim_object_descriptor_get(), Abstract01, Abstract01_Class, abstract01);

static void _abstract01_class_init(void *k)
{
	printf("abstract01 class init\n");
}

static void _abstract01_instance_init(void *o)
{
	printf("abstract01 instance init\n");
}

static void _abstract01_instance_deinit(void *o)
{
	printf("abstract01 instance deinit\n");
}

typedef struct _Abstract02
{
	Abstract01 parent;
} Abstract02;

typedef struct _Abstract02_Class
{
	Abstract01_Class parent;
	int (*color_get)(Abstract02 *o);
} Abstract02_Class;

ENESIM_OBJECT_ABSTRACT_BOILERPLATE(abstract01_descriptor_get(), Abstract02, Abstract02_Class, abstract02);

static void _abstract02_class_init(void *k)
{
	printf("abstract02 class init\n");
}

static void _abstract02_instance_init(void *o)
{
	printf("abstract02 instance init\n");
}

static void _abstract02_instance_deinit(void *o)
{
	printf("abstract02 instance deinit\n");
}

typedef struct _Object01
{
	Abstract02 parent;
	char *str;
} Object01;

typedef struct _Object01_Class
{
	Abstract02_Class parent;
} Object01_Class;

ENESIM_OBJECT_INSTANCE_BOILERPLATE(abstract02_descriptor_get(), Object01, Object01_Class, object01);
#define OBJECT01_CLASS(k) ENESIM_OBJECT_CLASS_CHECK(k, Object01_Class, object01_descriptor_get())
#define OBJECT01(o) ENESIM_OBJECT_INSTANCE_CHECK(o, Object01, object01_descriptor_get())

static void _object01_class_init(void *k)
{
	Object01_Class *klass = OBJECT01_CLASS(k);
	printf("object01 class init\n");
}

static void _object01_instance_init(void *o)
{
	Object01 *thiz = OBJECT01(o);

	thiz->str = strdup("object01 str attribute");
	printf("object01 instance init\n");
}

static void _object01_instance_deinit(void *o)
{
	Object01 *thiz = OBJECT01(o);
	printf("object01 instance deinit '%s'\n", thiz->str);
	free(thiz->str);
}

Abstract01 * object01_new(void)
{
	Abstract01 *ret;
	ret = ENESIM_OBJECT_INSTANCE_NEW(object01);
	return ret;
}

int main(void)
{
	Abstract01 *o;

	/* create a new object */
	o = object01_new();
	enesim_object_instance_free(o);
	return 0;
}
