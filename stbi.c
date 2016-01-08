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

int
stbex_pixel_compare_r(const stbex_pixel *lhs, const stbex_pixel *rhs)
{
	return lhs->b > rhs->b ? 1: -1;
}

int
stbex_pixel_compare_g(const stbex_pixel *lhs, const stbex_pixel *rhs)
{
	return lhs->g > rhs->g ? 1: -1;
}

int
stbex_pixel_compare_b(const stbex_pixel *lhs, const stbex_pixel *rhs)
{
	return lhs->b > rhs->b ? 1: -1;
}

void
stbex_pixel_sort_r(stbex_pixel * const pixels, size_t npixels)
{
	qsort(pixels, npixels, sizeof(stbex_pixel),
		  (int (*)(const void *, const void *))stbex_pixel_compare_r);
}

void
stbex_pixel_sort_g(stbex_pixel * const pixels, size_t npixels)
{
	qsort(pixels, npixels, sizeof(stbex_pixel),
		  (int (*)(const void *, const void *))stbex_pixel_compare_g);
}

void
stbex_pixel_sort_b(stbex_pixel * const pixels, size_t npixels)
{
	qsort(pixels, npixels, sizeof(stbex_pixel),
		  (int (*)(const void *, const void *))stbex_pixel_compare_b);
}


/*****************************************************************************
 *
 * Median cut
 *
 *****************************************************************************/

/** cube */
struct stbex_cube;
typedef struct _stbex_cube {
	uint8_t min_r;
	uint8_t min_g;
	uint8_t min_b;
	uint8_t max_r;
	uint8_t max_g;
	uint8_t max_b;
	size_t npixels;
	stbex_pixel *pixels;
	struct stbex_cube *left;
	struct stbex_cube *right;
	struct stbex_cube *parent;
} stbex_cube;

void
stbex_cube_fit(stbex_cube *cube)
{
	int i;
	stbex_pixel *p;

	cube->max_r = 0;
	cube->min_r = 255;
	cube->max_g = 0;
	cube->min_g = 255;
	cube->max_b = 0;
	cube->min_b = 255;

	for (i = 0; i < cube->npixels; i++) {
		p = cube->pixels + i;
		if (p->r < cube->min_r) {
			cube->min_r = p->r;
		}
		if (p->g < cube->min_g) {
			cube->min_g = p->g;
		}
		if (p->b < cube->min_b) {
			cube->min_b = p->b;
		}
		if (p->r > cube->max_r) {
			cube->max_r = p->r;
		}
		if (p->g > cube->max_g) {
			cube->max_g = p->g;
		}
		if (p->b > cube->max_b) {
			cube->max_b = p->b;
		}
	}
}

struct stbex_cube *
stbex_cube_new(stbex_pixel *pixels, size_t npixels, stbex_cube *parent)
{
	stbex_cube *cube;
   
	cube = malloc(sizeof(stbex_cube));
	cube->pixels = malloc(sizeof(stbex_pixel *) * npixels);
	memcpy(cube->pixels, pixels, sizeof(stbex_pixel *) * npixels);
	cube->npixels = npixels;
	cube->left = NULL;
	cube->right = NULL;
	cube->parent = (struct stbex_cube*)parent;

	stbex_cube_fit(cube);

	return (struct stbex_cube *)cube;
}

void
stbex_cube_free(stbex_cube *cube, stbex_pixel *pixels)
{
	free(cube->pixels);
	free(cube);
}

int
stbex_cube_hatch(stbex_cube *cube, int threshold)
{
	int length_r;
	int length_g;
	int length_b;
	int divide_point;
	int divide_value = 0;

	if (cube->left != NULL && cube->right != NULL) {
		return stbex_cube_hatch((stbex_cube *)cube->left, threshold)
			 + stbex_cube_hatch((stbex_cube *)cube->right, threshold);
	}

	length_r = (int)cube->max_r - (int)cube->min_r;
	length_g = (int)cube->max_g - (int)cube->min_g;
	length_b = (int)cube->max_b - (int)cube->min_b;

	if (cube->npixels <= 8) {
		return cube->npixels;
	}
		   
	if (cube->npixels < threshold) {
		if (length_r < 16 && length_g < 16 && length_b < 16) {
			return 1;
		}
		return 0;
	}

	divide_point = cube->npixels / 2;

	if (length_r > length_g && length_r > length_b) {
		stbex_pixel_sort_r(cube->pixels, cube->npixels);
		divide_value = cube->pixels[divide_point - 1].r;
		for (; divide_point < cube->npixels; divide_point++) {
			if (cube->pixels[divide_point].r != divide_value) {
				break;
			}
		}
	} else if (length_g > length_b) {
		stbex_pixel_sort_g(cube->pixels, cube->npixels);
		divide_value = cube->pixels[divide_point - 1].g;
		for (; divide_point < cube->npixels; divide_point++) {
			if (cube->pixels[divide_point].g != divide_value) {
				break;
			}
		}
	} else {
		stbex_pixel_sort_b(cube->pixels, cube->npixels);
		divide_value = cube->pixels[divide_point - 1].b;
		for (; divide_point < cube->npixels; divide_point++) {
			if (cube->pixels[divide_point].b != divide_value) {
				break;
			}
		}
	}

	if (divide_point == cube->npixels) {
		return 1;
	}

	if (cube->npixels == divide_point + 1) {
		return 1;
	}

	cube->left = stbex_cube_new(cube->pixels, divide_point, cube);
	cube->right = stbex_cube_new(cube->pixels + divide_point + 1, cube->npixels - divide_point - 1, cube);
	cube->npixels = 0;

	return 2;
}

void
stbex_cube_get_sample(stbex_cube *cube, stbex_pixel *samples, stbex_pixel *results, int *nresults)
{
	int length_r;
	int length_g;
	int length_b;

	if (cube->left) {
		stbex_cube_get_sample((stbex_cube *)cube->left, samples, results, nresults);
		stbex_cube_get_sample((stbex_cube *)cube->right, samples, results, nresults);
	} else {

		length_r = (int)cube->max_r - (int)cube->min_r;
		length_g = (int)cube->max_g - (int)cube->min_g;
		length_b = (int)cube->max_b - (int)cube->min_b;

		if (length_r < 16 && length_g < 16 && length_b < 16) {
			*(results + (*nresults)++) = stbex_pixel_new((cube->min_r + cube->max_r) / 2, (cube->min_g + cube->max_g) / 2, (cube->min_b + cube->max_b) / 2, 0); 
/*
*/
		} else {
			*(results + (*nresults)++) = stbex_pixel_new(cube->min_r, cube->min_g, cube->min_b, 0); 
			*(results + (*nresults)++) = stbex_pixel_new(cube->max_r, cube->min_g, cube->min_b, 0); 
			*(results + (*nresults)++) = stbex_pixel_new(cube->min_r, cube->max_g, cube->min_b, 0); 
			*(results + (*nresults)++) = stbex_pixel_new(cube->min_r, cube->min_g, cube->max_b, 0); 
			*(results + (*nresults)++) = stbex_pixel_new(cube->max_r, cube->max_g, cube->min_b, 0); 
			*(results + (*nresults)++) = stbex_pixel_new(cube->min_r, cube->max_g, cube->max_b, 0); 
			*(results + (*nresults)++) = stbex_pixel_new(cube->max_r, cube->min_g, cube->max_b, 0); 
			*(results + (*nresults)++) = stbex_pixel_new(cube->max_r, cube->max_g, cube->max_b, 0); 
		}
	}
}

/*****************************************************************************
 *
 * Coulor reduction
 *
 *****************************************************************************/

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

stbex_pixel *
zigzag_pget(unsigned char *data, int index, int width, int depth)
{
	int n = (int)floor(sqrt((index + 1) * 8) * 0.5 - 0.5);
	int x, y;

	if ((n & 0x1) == 0) {
		y = index - n * (n + 1) / 2;
		x = n - y;
	} else {
		x = index - n * (n + 1) / 2;
		y = n - x;
	}
	return (stbex_pixel *)(data + (y * width + x) * depth);
}

stbex_pixel *
get_sample(unsigned char *data, int width, int height, int depth, int *count)
{
	int i, j;
	int n = width * height / *count;
	int index;
	stbex_pixel *result = malloc(sizeof(stbex_pixel) * *count);
	stbex_pixel p;
	char histgram[1 << 15];

	memset(histgram, 0, sizeof(histgram));

	for (i = 0; i < *count; i++) {
		/* p = *zigzag_pget(data, i * n, width, depth); */
		p = *pget(data, i * n, depth);
		index = (p.r >> 3) << 10 | (p.g >> 3) << 5 | p.b >> 3;
		histgram[index] = 1;
	}

	for (i = 0, j = 0; i < sizeof(histgram); i++) {
		if (histgram[i] != 0) {
			result[j].r = (i >> 10 & 0x1f) << 3;
			result[j].g = (i >> 5 & 0x1f) << 3;
			result[j].b = (i & 0x1f) << 3;
			j++;
		}
	}
	*count = j;
	return result;
}

void add_offset(unsigned char *data, int i, int n, int roffset, int goffset, int boffset) {
	int r = data[i * n + 0] + roffset;
	int g = data[i * n + 1] + goffset;
	int b = data[i * n + 2] + boffset;

	if (r < 0) {
		r = 0;
	}
	if (g < 0) {
		g = 0;
	}
	if (b < 0) {
		b = 0;
	}
	if (r > 255) {
		r = 255;
	}
	if (g > 255) {
		g = 255;
	}
	if (b > 255) {
		b = 255;
	}

	data[i * n + 0] = (unsigned char)r;
	data[i * n + 1] = (unsigned char)g;
	data[i * n + 2] = (unsigned char)b;
}

/*****************************************************************************
 *
 * Image object
 *
 *****************************************************************************/

static PyObject* error;
/** Image object */
typedef struct _Image {
	PyObject_HEAD
	unsigned char *original;
	int width;
	int height;
	int depth;
} Image;

/** allocator */
static PyObject *
Image_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	Image *self = (Image *)type->tp_alloc(type, 0);
	if (self == NULL) {
		return NULL;
	}
	self->original = NULL;
	self->width = 0;
	self->height = 0;
	self->depth = 0;

	return (PyObject *)self;
}
 
/** deallocator */
static void
Image_dealloc(Image *self)
{
	if (self->original) {
		stbi_image_free(self->original);
	}

	Py_TYPE(self)->tp_free((PyObject*)self);
}

/** initializer */
/*
 static int
Image_init(Image *self, PyObject *args, PyObject *kwds)
{
	PyObject *tmp;
   
	static char *kwlist[] = {
		"width",
		"height",
		"depth",
		NULL
	};
   
	int result = PyArg_ParseTupleAndKeywords(args, kwds, "|iii", kwlist,
					                         &self->width,
					                         &self->height,
					                         &self->depth);
  
	if (!result) {
		return -1;
	}
   
	return 0;
}
*/

static PyMemberDef Image_members[] = {
	{ NULL } 
};


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
Image_write_png(PyObject* self, PyObject* args) {
	Image* img = (Image*)self;
	const char* file;
	int unused;
	if (!PyArg_ParseTuple(args, "s|i", &file, &unused)) {
		return NULL;
	}
	int r = stbi_write_png(file, img->width, img->height, img->depth, img->original, 0);

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
Image_thumbnail(PyObject *self, PyObject *args)
{
	PyTypeObject *type = (PyTypeObject *)PyObject_Type(self);

	Image *src = (Image *)self;
	Image *dest;
	int width, height;
	int gx, gy;
	int rx, ry;
	const size_t colors = 256;
	stbex_pixel *p;
	int unused = 0;
	if (!PyArg_ParseTuple(args, "(ii)i", &width, &height, &unused)) {
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

static PyMethodDef Image_methods[] = {
	{"clone", (PyCFunction)Image_clone, METH_KEYWORDS, "copy image data" },
	{"thumbnail", Image_thumbnail, METH_VARARGS, "resize image data" },
	{"crop", Image_crop, METH_VARARGS, "cut image" },
	{"gray", Image_gray, METH_NOARGS, "gray image" },
	{"save", Image_write_png, METH_VARARGS|METH_KEYWORDS, "save image to file as png format" },
	{ NULL }  /* Sentinel */
};

static PyObject * Image_data(Image* self, void* closure) {
	return PyByteArray_FromStringAndSize(self->original, self->width * self->height * self->depth);
}

static PyObject *
Image_getsize(Image *self, void *closure)
{
	PyObject *width = PyInt_FromLong(self->width);
	PyObject *height = PyInt_FromLong(self->height);
	return PyTuple_Pack(2, width, height);
}

static PyGetSetDef Image_getseters[] = {
	{ "size", (getter)Image_getsize, NULL, "size", NULL },
	{ "data", (getter)Image_data, NULL, "data", NULL },
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
	Image_new,				                /* tp_new */
};
 
/*
*/
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
			char buffer[1024] = {0};
			sprintf(buffer, "%s(%s %d) - stbi_load() is failure\n", __FUNCTION__, __FILE__, __LINE__);			PyErr_SetString(error, buffer);
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

static char Image_doc[] = "A simple python image library which likes PIL.\n";

static PyMethodDef methods[] = {
	{ "open", open_image, METH_VARARGS, "load image by file\n" },
	{ NULL, NULL, 0, NULL}
};

/** module entry point */
PyMODINIT_FUNC initstbi(void)
{
	PyObject *m = Py_InitModule3("stbi", methods, Image_doc);
	if (PyType_Ready(&ImageType) < 0) {
		return;
	}
	PyModule_AddObject(m, "image", (PyObject *)&ImageType);
	PyModule_AddObject(m, "ANTIALIAS", Py_BuildValue("i", 0));
	error = PyErr_NewException("stbi.error", NULL, NULL);
	Py_INCREF(error);
	PyModule_AddObject(m, "error", error);
}

// EOF
