#pragma warning(disable:4005)
#include <Python.h>
#include <structmember.h>

#include <stdlib.h>
#include <stdint.h>
#include <memory.h>
#include <math.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb/stb_image_resize.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"


/*****************************************************************************
 *
 * Pixel object
 *
 *****************************************************************************/

typedef struct _stbex_pixel {
	union {
		struct {
			uint8_t r;
			uint8_t g;
			uint8_t b;
			uint8_t a;
		};
		uint32_t color_index;
	};
} stbex_pixel;

stbex_pixel
stbex_pixel_new(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	stbex_pixel p;

	p.r = r;
	p.g = g;
	p.b = b; 
	p.a = a;

	return p;
}

void
pset(uint8_t *data, int index, int depth, stbex_pixel *value)
{
	memcpy(data + index * depth, value, depth);
}

stbex_pixel *
pget(unsigned char *data, int index, int depth)
{
	return (stbex_pixel *)(data + index * depth);
}

/*****************************************************************************
 *
 * Image object
 *
 *****************************************************************************/

/** Image object */
typedef struct _Image {
	PyObject_HEAD
	unsigned char *original;
	int width;
	int height;
	int depth;
} Image;

/** deallocator */
static void
Image_dealloc(Image *self)
{
	if (self->original) {
		stbi_image_free(self->original);
	}

	Py_TYPE(self)->tp_free((PyObject*)self);
}


/* copy */
static PyObject *
Image_clone(PyObject *self, PyObject *args, PyObject *kwds)
{
	PyTypeObject *type = (PyTypeObject *)PyObject_Type(self);

	if (type == NULL) {
		return NULL;
	}

	Image *src = (Image *)self;
	Image *dest = (Image *)type->tp_alloc(type, 0);

	if (dest == NULL) {
		return NULL;
	}

	dest->width = src->width;
	dest->height = src->height;
	dest->depth = src->depth;
	dest->original = malloc(src->width * src->height * src->depth);
	memcpy(dest->original, src->original, src->width * src->height * src->depth);

	return (PyObject *)dest;
}

static PyObject *
Image_save(PyObject* self, PyObject* args, PyObject* keywords) {
	int r; {
		Image* img = (Image*)self;
		const char* file; const char* format = NULL; {
			static char* kwds[] = {"file", "format", NULL};
			if (!PyArg_ParseTupleAndKeywords(args, keywords, "s|s", kwds, &file, &format)) {
				{char buffer[1024] = {0};sprintf(buffer, "%s(%s %d) - invaild arguments\n", __FUNCTION__, __FILE__, __LINE__);PyErr_SetString(PyExc_TypeError, buffer);}
				return NULL;
			}
		}
		if (format == NULL) {
			r = stbi_write_png(file, img->width, img->height, img->depth, img->original, 0);
		}
		else {
			if (strcmp(format, "bmp") == 0) {
				r = stbi_write_bmp(file, img->width, img->height, img->depth, img->original);
			}
			else if (strcmp(format, "tga") == 0) {
				r = stbi_write_tga(file, img->width, img->height, img->depth, img->original);
			}
			else if (strcmp(format, "hdr") == 0) {
				r = stbi_write_hdr(file, img->width, img->height, img->depth, (float*)img->original);
			}
			else { //png or invalid format arguments
				r = stbi_write_png(file, img->width, img->height, img->depth, img->original, 0);
			}
		}
	}
	if (r == 0) {
		//error
		return NULL;
	}
	Py_RETURN_NONE;
}

static PyObject* Image_gray(PyObject* self, PyObject* args) {
	PyTypeObject *type = (PyTypeObject *)PyObject_Type(self);
	Image *src = (Image *)self;
	Image *dest = (Image *)type->tp_alloc(type, 0);
	if (dest == NULL) {
		printf("%s(%s %d) - failure to alloc image\n", __FUNCTION__, __FILE__, __LINE__);
		return NULL;
	}
	dest->width = src->width;
	dest->height = src->height;
	dest->depth = 1;
	dest->original = malloc(dest->width * dest->height * dest->depth);
	for (int gy = 0; gy < src->height; ++gy) {
		for (int gx = 0; gx < src->width; ++gx) {
			stbex_pixel *p;
			p = pget(src->original, src->width * gy + gx, src->depth);
			*(dest->original + dest->width * gy + gx) = (p->r * 30 + p->g * 59 + p->b * 11) / 100;
		}
	}
	return (PyObject *)dest;


}
static PyObject *
Image_crop(PyObject *self, PyObject *args)
{
	PyTypeObject *type = (PyTypeObject *)PyObject_Type(self);

	Image *src = (Image *)self;
	Image *dest;
	int left, top, right, bottom;
	int x, y, width, height;
	int gx, gy;
	int rx, ry;
	const size_t colors = 256;
	stbex_pixel *p;

	if (!PyArg_ParseTuple(args, "iiii", &left, &top, &right, &bottom)) {
		printf("%s(%s %d) - paramater error\n", __FUNCTION__, __FILE__, __LINE__);
		return NULL;
	}
	width = right - left;
	height = bottom - top;
	x = left;
	y = top;
	dest = (Image *)type->tp_alloc(type, 0);
	if (dest == NULL) {
		printf("%s(%s %d) - failure to alloc image\n", __FUNCTION__, __FILE__, __LINE__);
		return NULL;
	}
	dest->depth = src->depth;
	dest->original = malloc(width * height * src->depth);
	for (gy = 0; gy < height; ++gy) {
		for (gx = 0; gx < width; ++gx) {
			p = pget(src->original, src->width * (y + gy) + x + gx, src->depth);
			pset(dest->original, width * gy + gx, dest->depth, p);
		}
	}
	dest->width = width;
	dest->height = height;

	return (PyObject *)dest;
}

static PyObject *
Image_resize(PyObject *self, PyObject *args)
{
	PyTypeObject *type = (PyTypeObject *)PyObject_Type(self);

	Image *src = (Image *)self;
	Image *dest;
	int width, height;
	int gx, gy;
	int rx, ry;
	const size_t colors = 256;
	stbex_pixel *p;
	if (!PyArg_ParseTuple(args, "ii", &width, &height)) {
		printf("%s(%s %d) - paramater error\n", __FUNCTION__, __FILE__, __LINE__);
		return NULL;
	}
	dest = (Image *)type->tp_alloc(type, 0);
	if (dest == NULL) {
		printf("%s(%s %d) - failure to alloc image\n", __FUNCTION__, __FILE__, __LINE__);
		return NULL;
	}
	dest->depth = src->depth;
	dest->original = malloc(width * height * src->depth);
	int rlt = stbir_resize_uint8(src->original, src->width, src->height, 0, dest->original, width, height, 0, dest->depth);

	dest->width = width;
	dest->height = height;

	return (PyObject *)dest;
}

static PyObject * Image_data(Image* self, void* closure) {
	return PyByteArray_FromStringAndSize(self->original, self->width * self->height * self->depth);
}

static int Image_set_data(Image* self, PyObject* value, void* closure) {
	if (value == NULL) {
		return -1;
	}
	if (!PyByteArray_Check(value)) {
		return -1;
	}
	printf("%p\n", self->original);
	free(self->original);
	self->original = PyByteArray_AsString(value);
	return 0;
}

static PyObject *
Image_getsize(Image *self, void *closure)
{
	PyObject *width = PyInt_FromLong(self->width);
	PyObject *height = PyInt_FromLong(self->height);
	return PyTuple_Pack(2, width, height);
}

static PyMemberDef Image_members[] = {
	{ NULL } 
};

static PyMethodDef Image_methods[] = {
	{"clone", (PyCFunction)Image_clone, METH_KEYWORDS, "copy image data" },
	{"resize", Image_resize, METH_VARARGS, "resize image data" },
	{"crop", Image_crop, METH_VARARGS, "cut image" },
	{"gray", Image_gray, METH_NOARGS, "gray image" },
	{"save", (PyCFunction)Image_save, METH_VARARGS|METH_KEYWORDS, "save image to file as png format" },
	{ NULL }  /* Sentinel */
};

static PyGetSetDef Image_getseters[] = {
	{ "size", (getter)Image_getsize, NULL, "size", NULL },
	{ "data", (getter)Image_data, (setter)Image_set_data, "data", NULL },
	{ NULL }  /* Sentinel */
}; 

static PyTypeObject ImageType = {
	PyObject_HEAD_INIT(NULL)
	0,				                        /*ob_size*/
	"stbi.Image",				             /*tp_name*/
	sizeof(Image),				            /*tp_basicsize*/
	0,				                        /*tp_itemsize*/
	(destructor)Image_dealloc,				/*tp_dealloc*/
	0,				                        /*tp_print*/
	0,				                        /*tp_getattr*/
	0,				                        /*tp_setattr*/
	0,				                        /*tp_compare*/
	0,				                        /*tp_repr*/
	0,				                        /*tp_as_number*/
	0,				                        /*tp_as_sequence*/
	0,				                        /*tp_as_mapping*/
	0,				                        /*tp_hash */
	0,				                        /*tp_call*/
	0,				                        /*tp_str*/
	0,				                        /*tp_getattro*/
	0,				                        /*tp_setattro*/
	0,				                        /*tp_as_buffer*/
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
	"STB Image class",				        /* tp_doc */
	0,				                        /* tp_traverse */
	0,				                        /* tp_clear */
	0,				                        /* tp_richcompare */
	0,				                        /* tp_weaklistoffset */
	0,				                        /* tp_iter */
	0,				                        /* tp_iternext */
	Image_methods,				            /* tp_methods */
	Image_members,				            /* tp_members */
	Image_getseters,				          /* tp_getset */
	0,				                        /* tp_base */
	0,				                        /* tp_dict */
	0,				                        /* tp_descr_get */
	0,				                        /* tp_descr_set */
	0,				                        /* tp_dictoffset */
	0,//(initproc)Image_init,				     /* tp_init */
	0,				                        /* tp_alloc */
	0,				                /* tp_new */
};
 
static PyObject *
open_image(PyObject *self, PyObject *args)
{
	int x, y, n;
	PyObject *file;
	PyObject *chunk;
	unsigned char *buffer;
	long length;
	char *filename;
	const char *tp_name;
	unsigned char *data;
	const size_t colors = 256;

	if (!PyArg_ParseTuple(args, "O", &file)) {
		return NULL;
	}
	
	tp_name = file->ob_type->tp_name;

	if (strcmp(tp_name, "str") == 0) {
		filename = PyString_AsString(file);
		data = stbi_load(filename, &x, &y, &n, STBI_default);
		if (data == NULL) {
			{char buffer[1024] = {0};sprintf(buffer, "%s(%s %d) - stbi_load() is failure\n", __FUNCTION__, __FILE__, __LINE__);PyErr_SetString(PyExc_TypeError, buffer);}
			return NULL;
		}
	} else if (strcmp(tp_name, "cStringIO.StringI") == 0) {
		chunk = PyObject_CallMethod(file, "getvalue", NULL);
		if (chunk == NULL) {
			return NULL;
		}
		PyString_AsStringAndSize(chunk, (char **)&buffer, &length);
		data = stbi_load_from_memory(buffer, length, &x, &y, &n, STBI_default);
		if (data == NULL) {
			return NULL;
		}
	} else {
		return NULL;
	}


	Image *pimage = (Image *)ImageType.tp_alloc(&ImageType, 0);
	if (pimage == NULL) {
		return NULL;
	}

	pimage->original = data;
	pimage->width = x;
	pimage->height = y;
	pimage->depth = n;
	return (PyObject *)pimage;
}

static char stbi_doc[] = "A simple python image library which likes PIL.\n";

static PyMethodDef stbi_methods[] = {
	{ "open", open_image, METH_VARARGS, "load image from file or input stream(StringI)\n" },
	{ NULL, NULL, 0, NULL}
};

/** module entry point */
PyMODINIT_FUNC initstbi(void)
{
	PyObject *m = Py_InitModule3("stbi", stbi_methods, stbi_doc);
	if (PyType_Ready(&ImageType) < 0) {
		return;
	}
	PyModule_AddObject(m, "image", (PyObject *)&ImageType);
}

// EOF
