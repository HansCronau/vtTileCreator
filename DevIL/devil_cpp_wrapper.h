// This is the top part, the actual header part, of devil_cpp_wrapper.hpp.
// The bottom part was moved to devil_cpp_wrapper.cpp.

#ifndef DEVIL_CPP_WRAPPER_H
#define DEVIL_CPP_WRAPPER_H

#include <IL/ilut.h>

class ilImage
{
public:
	ilImage();
	ilImage(ILconst_string);
	ilImage(const ilImage &);
	virtual		~ilImage();

	ILboolean	Load(ILconst_string);
	ILboolean	Load(ILconst_string, ILenum);
	ILboolean	Save(ILconst_string);
	ILboolean	Save(ILconst_string, ILenum);

	// ImageLib functions
	ILboolean	ActiveImage(ILuint);
	ILboolean	ActiveLayer(ILuint);
	ILboolean	ActiveMipmap(ILuint);
	ILboolean	Clear(void);
	void		ClearColour(ILclampf, ILclampf, ILclampf, ILclampf);
	ILboolean	Convert(ILenum);
	ILboolean	Copy(ILuint);
	ILboolean	Default(void);
	ILboolean	Flip(void);
	ILboolean	SwapColours(void);
	ILboolean	Resize(ILuint, ILuint, ILuint);
	ILboolean	TexImage(ILuint, ILuint, ILuint, ILubyte, ILenum, ILenum, void*);

	// Image handling
	void		Bind(void) const;
	void		Bind(ILuint);
	void		Close(void);
	void		Delete(void);
	void		iGenBind();
	ILenum		PaletteAlphaIndex();

	// Image characteristics
	ILuint		Width(void);
	ILuint		Height(void);
	ILuint		Depth(void);
	ILubyte		Bpp(void);
	ILubyte		Bitpp(void);
	ILenum		PaletteType(void);
	ILenum		Format(void);
	ILenum		Type(void);
	ILuint		NumImages(void);
	ILuint		NumMipmaps(void);
	ILuint		GetId(void) const;
	ILenum      GetOrigin(void);
	ILubyte		*GetData(void);
	ILubyte		*GetPalette(void);

	// Rendering
	ILuint		BindImage(void);
	ILuint		BindImage(ILenum);

	// Operators
	ilImage&	operator = (ILuint);
	ilImage&	operator = (const ilImage &);

protected:
	ILuint		Id;

private:
	static int		ilStartUp();
	static ILboolean	ilStartedUp;
};

class ilFilters
{
public:
	static ILboolean	Alienify(ilImage &);
	static ILboolean	BlurAvg(ilImage &, ILuint Iter);
	static ILboolean	BlurGaussian(ilImage &, ILuint Iter);
	static ILboolean	Contrast(ilImage &, ILfloat Contrast);
	static ILboolean	EdgeDetectE(ilImage &);
	static ILboolean	EdgeDetectP(ilImage &);
	static ILboolean	EdgeDetectS(ilImage &);
	static ILboolean	Emboss(ilImage &);
	static ILboolean	Gamma(ilImage &, ILfloat Gamma);
	static ILboolean	Negative(ilImage &);
	static ILboolean	Noisify(ilImage &, ILubyte Factor);
	static ILboolean	Pixelize(ilImage &, ILuint PixSize);
	static ILboolean	Saturate(ilImage &, ILfloat Saturation);
	static ILboolean	Saturate(ilImage &, ILfloat r, ILfloat g, ILfloat b, ILfloat Saturation);
	static ILboolean	ScaleColours(ilImage &, ILfloat r, ILfloat g, ILfloat b);
	static ILboolean	Sharpen(ilImage &, ILfloat Factor, ILuint Iter);
};

#ifdef ILUT_USE_OPENGL
class ilOgl
{
public:
	static void			Init(void);
	static GLuint		BindTex(ilImage &);
	static ILboolean	Upload(ilImage &, ILuint);
	static GLuint		Mipmap(ilImage &);
	static ILboolean	Screen(void);
	static ILboolean	Screenie(void);
};
#endif//ILUT_USE_OPENGL


#ifdef ILUT_USE_ALLEGRO
class ilAlleg
{
public:
	static void	Init(void);
	static BITMAP	*Convert(ilImage &);
};
#endif//ILUT_USE_ALLEGRO


#ifdef ILUT_USE_WIN32
class ilWin32
{
public:
	static void		Init(void);
	static HBITMAP		Convert(ilImage &);
	static ILboolean	GetClipboard(ilImage &);
	static void		GetInfo(ilImage &, BITMAPINFO *Info);
	static ILubyte		*GetPadData(ilImage &);
	static HPALETTE		GetPal(ilImage &);
	static ILboolean	GetResource(ilImage &, HINSTANCE hInst, ILint ID, char *ResourceType);
	static ILboolean	GetResource(ilImage &, HINSTANCE hInst, ILint ID, char *ResourceType, ILenum Type);
	static ILboolean	SetClipboard(ilImage &);
};
#endif//ILUT_USE_WIN32


class ilValidate
{
public:
	static ILboolean	Valid(ILenum, ILconst_string);
	static ILboolean	Valid(ILenum, FILE *);
	static ILboolean	Valid(ILenum, void *, ILuint);

protected:

private:

};

class ilState
{
public:
	static ILboolean		Disable(ILenum);
	static ILboolean		Enable(ILenum);
	static void				Get(ILenum, ILboolean &);
	static void				Get(ILenum, ILint &);
	static ILboolean		GetBool(ILenum);
	static ILint			GetInt(ILenum);
	static const char		*GetString(ILenum);
	static ILboolean		IsDisabled(ILenum);
	static ILboolean		IsEnabled(ILenum);
	static ILboolean		Origin(ILenum);
	static void				Pop(void);
	static void				Push(ILuint);

protected:

private:

};


class ilError
{
public:
	static void			Check(void(*Callback)(const char*));
	static void			Check(void(*Callback)(ILenum));
	static ILenum		Get(void);
	static const char	*String(void);
	static const char	*String(ILenum);

protected:

private:

};

#endif // DEVIL_CPP_WRAPPER_H
