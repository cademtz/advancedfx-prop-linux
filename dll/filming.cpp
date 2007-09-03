
#include "wrect.h"
#include "cl_dll.h"
#include "cdll_int.h"
#include "r_efx.h"
#include "com_model.h"
#include "r_studioint.h"
#include "pm_defs.h"
#include "cvardef.h"
#include "entity_types.h"

#include "cmdregister.h"
#include "filming.h"

//#include "math.h"

extern cl_enginefuncs_s *pEngfuncs;
extern engine_studio_api_s *pEngStudio;
extern playermove_s *ppmove;

extern float clamp(float, float, float);

REGISTER_CVAR(crop_height, "-1", 0);
REGISTER_CVAR(crop_yofs, "-1", 0);

REGISTER_DEBUGCVAR(depth_bias, "0", 0);
REGISTER_CVAR(depth_logarithmic, "32", 0);
REGISTER_DEBUGCVAR(depth_scale, "1", 0);

REGISTER_DEBUGCVAR(gl_force_noztrick, "1", 0);

REGISTER_CVAR(movie_customdump, "1", 0)
REGISTER_CVAR(movie_depthdump, "0", 0);
REGISTER_CVAR(movie_filename, "untitled", 0);
REGISTER_CVAR(movie_splitstreams, "0", 0);
REGISTER_CVAR(movie_swapweapon, "0", 0);
REGISTER_CVAR(movie_swapdoors, "0", 0);
REGISTER_CVAR(movie_onlyentity, "0", 0);
REGISTER_CVAR(movie_clearscreen, "0", 0);
REGISTER_CVAR(movie_fps, "30", 0);
REGISTER_CVAR(movie_wireframe, "0", 0);
REGISTER_CVAR(movie_wireframesize, "1", 0);

// Our filming singleton
Filming g_Filming;

void Filming::setScreenSize(GLint w, GLint h)
{
	if (m_iWidth < w) m_iWidth = w;
	if (m_iHeight < h) m_iHeight = h;
}

void Filming::Start()
{
	// Different filename, so save it and reset the take count
	if (strncmp(movie_filename->string, m_szFilename, sizeof(m_szFilename) - 1) != 0)
	{
		strncpy(m_szFilename, movie_filename->string, sizeof(m_szFilename) - 1);
		m_nTakes = 0;
	}

	m_nFrames = 0;
	m_iFilmingState = FS_STARTING;
	m_iMatteStage = MS_WORLD;

	// Init Cropping:
	// retrive cropping settings and make sure the values are in bounds we can work with:
	// -1 means the code will try to act as if this value was default (default ofs / default max height)

	int iTcrop_yofs=(int)crop_yofs->value;

	m_iCropHeight = (int)crop_height->value;
	if (m_iCropHeight == -1) m_iCropHeight = m_iHeight; // default is maximum height we know, doh :O
	else
	{
		// make sure we are within valid bounds to avoid mem acces errors and potential problems with badly coded loops :P
		// may be someone can optimize this:
		if (m_iCropHeight > m_iHeight) m_iCropHeight=m_iHeight;
		else if (m_iCropHeight < 2)  m_iCropHeight=2;
	}

	if (iTcrop_yofs==-1) m_iCropYOfs = (m_iHeight-m_iCropHeight)/2; // user wants that we center the height-crop (this way we preffer cutting off top lines (OpenGL y-axis!) if the number of lines is uneven)
	else {
		int iTHeightDiff = (m_iHeight - m_iCropHeight);
		// user specified an offset, we will transform the values for him, so he sees Yofs/-axis as top->down while OpenGL handles it down->up
		m_iCropYOfs  =  iTHeightDiff - iTcrop_yofs; //GL y-axis is down->up

		// may be someone can optimize this:
		if (m_iCropYOfs > iTHeightDiff) m_iCropYOfs=iTHeightDiff;
		else if (m_iCropYOfs<0) m_iCropYOfs=0;
	}


	// prepare (and force) some HL engine settings:

	// Clear up the screen
	if (movie_clearscreen->value != 0.0f)
	{
		pEngfuncs->Cvar_SetValue("hud_draw", 0);
		pEngfuncs->Cvar_SetValue("crosshair", 0);
	}

	//
	// gl_ztrick:
	// we force it to 0 by default, cause otherwise it could suppress gl_clear and mess up, see ID SOftware's Quake 1 source for more info why this has to be done
	if (gl_force_noztrick->value)
		pEngfuncs->Cvar_SetValue("gl_ztrick", 0);

	// well for some reason gavin forced gl_clear 1, but we don't relay on it anyways (which is good, cause the in ineye demo mode the engine will reforce it to 0 anyways):
	pEngfuncs->Cvar_SetValue("gl_clear", 1); // this needs should be reforced somwhere since in ineydemo mode the engine might force it to 0
}

void Filming::Stop()
{
	m_nFrames = 0;
	m_iFilmingState = FS_INACTIVE;
	m_nTakes++;

	// Need to reset this otherwise everything will run crazy fast
	pEngfuncs->Cvar_SetValue("host_framerate", 0);
}

void Filming::Capture(const char *pszFileTag, int iFileNumber, BUFFER iBuffer)
{
	char cDepth = (iBuffer == COLOR ? 2 : 3);
	int iMovieBitDepth = (int)(movie_depthdump->value);

	GLenum eGLBuffer = (iBuffer == COLOR ? GL_BGR_EXT : GL_DEPTH_COMPONENT);
	GLenum eGLtype = ((iBuffer == COLOR) ? GL_UNSIGNED_BYTE : GL_FLOAT);
	int nBits = ((iBuffer == COLOR ) ? 3 : (iMovieBitDepth==32?4:(iMovieBitDepth==24?3:(iMovieBitDepth==16?2:1))));

	char szFilename[128];
	_snprintf(szFilename, sizeof(szFilename) - 1, "%s_%s_%02d_%05d.tga", m_szFilename, pszFileTag, m_nTakes, iFileNumber);

	unsigned char szTgaheader[12] = { 0, 0, cDepth, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	unsigned char szHeader[6] = { (int) (m_iWidth % 256), (int) (m_iWidth / 256), (int) (m_iCropHeight % 256), (int) (m_iCropHeight / 256), 8 * nBits, 0 };

	FILE *pImage;

	//// in case we want to check if the buffer's are set:
	//GLint iTemp,iTemp2; glGetIntegerv(GL_READ_BUFFER,&iTemp); glGetIntegerv(GL_DRAW_BUFFER,&iTemp2); pEngfuncs->Con_Printf(">>Read:  0x%08x, Draw:  0x%08x \n",iTemp,iTemp2);

	bool bReadOk = m_GlRawPic.DoGlReadPixels(0, m_iCropYOfs, m_iWidth, m_iCropHeight, eGLBuffer, eGLtype);
	if (!bReadOk)
	{
		pEngfuncs->Con_Printf("MDT ERROR: failed to capture take %05d, Errorcode: %d.\n",m_nTakes,m_GlRawPic.GetLastUnhandledError());
		return; // may be we should code some better error handling here heh
	}

	// apply postprocessing to the depthbuffer:
	// the following code should be replaced completly later, cause it's rather dependent
	// on sizes of unsigned int and float and stuff, although it should be somewhat
	// save from acces violations (only corrupted pixel data):
	// also the GL_UNSIGNED_INT FIX is somewhat slow by now, code has to be optimized
	if (iBuffer==DEPTH)
	{
		// user wants 24 Bit output, we need to cut off
		int iSize=m_iWidth*m_iCropHeight;
		unsigned char* t_pBuffer=m_GlRawPic.GetPointer();	// the pointer where we write
		unsigned char* t_pBuffer2=t_pBuffer;				// the pointer where we read
		
		float tfloat;
		unsigned int tuint;

		unsigned int mymax=(1<<(8*nBits))-1;

		float logscale=depth_logarithmic->value;

		// these values are needed in order to work around in bugs i.e. of the NVIDIA Geforce 5600, since it doesn't use the default values for some reason:
		static float fHard_GL_DEPTH_BIAS  = 0.0; // OGL reference says default is 0.0
		static float fHard_GL_DEPTH_SCALE = 1.0; // OGL reference says default is 1.0	
		glGetFloatv(GL_DEPTH_BIAS,&fHard_GL_DEPTH_BIAS);
		glGetFloatv(GL_DEPTH_SCALE,&fHard_GL_DEPTH_SCALE);

		//pEngfuncs->Con_Printf("Depth: Scale: %f, Bias: %f, Firstpixel: %f\n",fHard_GL_DEPTH_SCALE,fHard_GL_DEPTH_BIAS,(float)*(float *)t_pBuffer2);

		for (int i=0;i<iSize;i++)
		{
			memmove(&tfloat,t_pBuffer2,sizeof(float));

            tfloat=tfloat*(depth_scale->value)+(depth_bias->value); // allow custom scale and offset
			tfloat=tfloat/fHard_GL_DEPTH_SCALE-fHard_GL_DEPTH_BIAS; // fix the range for card's that don't do it their selfs, although the OpenGL reference says so.

			if (logscale!=0)
			{
				tfloat = (exp(tfloat*logscale)-1)/(exp((float)1*logscale)-1);
			}
			tfloat*=mymax; // scale to int's max. value
			tuint = max(min((unsigned int)tfloat,mymax),0); // floor,clamp and convert to the desired data type

			memmove(t_pBuffer,&tuint,nBits);
				
			t_pBuffer+=nBits;
			t_pBuffer2+=sizeof(float);
		}
	}

	if ((pImage = fopen(szFilename, "wb")) != NULL)
	{
		fwrite(szTgaheader, sizeof(unsigned char), 12, pImage);
		fwrite(szHeader, sizeof(unsigned char), 6, pImage);

		fwrite(m_GlRawPic.GetPointer(), sizeof(unsigned char), m_iWidth * m_iCropHeight * nBits, pImage);

		fclose(pImage);
	}
}

Filming::DRAW_RESULT Filming::shouldDraw(GLenum mode)
{
	if (m_iMatteStage == MS_ALL)
		return DR_NORMAL;

	else if (m_iMatteStage == MS_ENTITY)
		return shouldDrawDuringEntityMatte(mode);

	else 
		return shouldDrawDuringWorldMatte(mode);
}

Filming::DRAW_RESULT Filming::shouldDrawDuringEntityMatte(GLenum mode)
{
	bool bSwapWeapon = (movie_swapweapon->value != 0);
	bool bSwapDoors = (movie_swapdoors->value != 0);
	int iOnlyActor = (int) movie_onlyentity->value;

	// GL_POLYGON is a worldbrush
	if (mode == GL_POLYGON)
	{
		cl_entity_t *ce = pEngStudio->GetCurrentEntity();

		// This is a polygon func_ something, so probably a door or a grill
		// We don't touch doors here ol' chap if swapdoors is on
		if (bSwapDoors && ce && ce->model && ce->model->type == mod_brush && strncmp(ce->model->name, "maps/", 5) != 0)
			return DR_NORMAL;

		return DR_MASK;
	}

	// Sprites and sky are just removed completely
	else if (mode == GL_QUADS)
		return DR_HIDE;

	// Entities
	else if (mode == GL_TRIANGLE_STRIP || mode == GL_TRIANGLE_FAN)
	{
		cl_entity_t *ce = pEngStudio->GetCurrentEntity();

		// Studio models
		if (ce && ce->model && ce->model->type == mod_studio)
		{
			// This is the viewmodel so hide it from ent-only if they want it to be shown as normal
			// However hide it via a mask so it still covers stuff
			// Actually do we want to do that?
			// edit: No, not for now as it breaks
			if (bSwapWeapon && strncmp("models/v_", ce->model->name, 9) == 0)
				return DR_HIDE;

			// We have selected 1 ent only to be visible and its not this
			if (iOnlyActor != 0 && iOnlyActor != ce->index)
				return DR_HIDE;
		}	
		// This is some sort of func thing so matte effect it
		// TODO: why is this doing MATTE_COLOUR instea of masking?
		else
			//glColor3f(MATTE_COLOUR);
			return DR_MASK;
	}

	return DR_NORMAL;
}

Filming::DRAW_RESULT Filming::shouldDrawDuringWorldMatte(GLenum mode)
{
	bool bSwapWeapon = (movie_swapweapon->value != 0);
	bool bSwapDoors = (movie_swapdoors->value != 0);
	int iOnlyActor = (int) movie_onlyentity->value;

	// Worldbrush stuff
	if (mode == GL_POLYGON)
	{
		cl_entity_t *ce = pEngStudio->GetCurrentEntity();

		// This is a polygon func_ something, so probably a door or a grill
		if (bSwapDoors && ce && ce->model && ce->model->type == mod_brush && strncmp(ce->model->name, "maps/", 5) != 0)
			return DR_HIDE;
	}

	// Entities...
	// We remove stuff rather than hide it, because in world only they probably
	// want the depth dump to just be the world!
	else if (mode == GL_TRIANGLE_STRIP || mode == GL_TRIANGLE_FAN)
	{
		cl_entity_t *ce = pEngStudio->GetCurrentEntity();

		// Studio models need only apply
		if (ce && ce->model && ce->model->type == mod_studio)
		{
			bool bKeepDueToSpecialCondition = false;

			// This is the viewmodel so hide it from ent-only if they want it to be shown as normal
			if (bSwapWeapon && strncmp("models/v_", ce->model->name, 9) == 0)
				return DR_NORMAL;

			// We have selected 1 ent only to be on the ent only layer and its not this
			if (iOnlyActor != 0 && iOnlyActor != ce->index)
				return DR_NORMAL;

			if (!bKeepDueToSpecialCondition)
				return DR_HIDE;
		}	
	}

	return DR_NORMAL;
}

void Filming::recordBuffers()
{
	// If this is a none swapping one then force to the correct stage.
	// Otherwise continue working wiht the stage that this frame has
	// been rendered with.
	if (movie_splitstreams->value < 3.0f)
		m_iMatteStage = (MATTE_STAGE) ((int) MS_ALL + (int) max(movie_splitstreams->value, 0.0f));

	// If we've only just started, delay until the next scene so that
	// the first frame is drawn correctly
	if (m_iFilmingState == FS_STARTING)
	{
		glClearColor(m_MatteColour[0], m_MatteColour[1], m_MatteColour[2], 1.0f); // don't forget to set our clear color
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
		m_iFilmingState = FS_ACTIVE;
		return;
	}

	bool bSplitting = (movie_splitstreams->value == 3.0f);
	float flTime = 1.0f / max(movie_fps->value, 1.0f);

	static char *pszTitles[] = { "all", "world", "entity" };
	static char *pszDepthTitles[] = { "depthall", "depthworld", "depthall" };

	// Are we doing our own screenshot stuff
	bool bCustomDumps = (movie_customdump->value != 0);
	bool bDepthDumps = (movie_depthdump->value != 0);

	if (bCustomDumps)
		Capture(pszTitles[m_iMatteStage], m_nFrames, COLOR);

	if (bDepthDumps)
		Capture(pszDepthTitles[m_iMatteStage], m_nFrames, DEPTH);

	float flNextFrameDuration = flTime;

	// If splitting, fill out the rest of the fps
	if (bSplitting)
	{
		// We want as little time to pass as possible until the next frame
		// is drawn, so the difference is tiny.
		if (m_iMatteStage == MS_WORLD)
		{
			flNextFrameDuration = MIN_FRAME_DURATION;
			m_iMatteStage = MS_ENTITY;
		}
		// Make up the rest of the time so that their fps is met.
		// Also increase the frame count
		else
		{
			flNextFrameDuration = flTime - MIN_FRAME_DURATION;
			m_iMatteStage = MS_WORLD;
			m_nFrames++;
		}
	}
	else
		m_nFrames++;

	// Make sure the next frame time isn't invalid
	flNextFrameDuration = max(flNextFrameDuration, MIN_FRAME_DURATION);

	pEngfuncs->Cvar_SetValue("host_framerate", flNextFrameDuration);
}

void Filming::clearBuffers()
{
	// Now we do our clearing!
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	// well for some reason gavin forced gl_clear 1, but we don't relay on it anyways (which is good, cause the in ineye demo mode the engine will reforce it to 0 anyways):
	pEngfuncs->Cvar_SetValue("gl_clear", 1); // reforce (I am not sure if this is a good position)
}

bool Filming::checkClear(GLbitfield mask)
{
	// we now want coll app controll
	// Don't clear unless we specify
	if (isFilming() && (mask & GL_COLOR_BUFFER_BIT || mask & GL_DEPTH_BUFFER_BIT))
		return false;

	// Make sure the mask colour is still correct
	glClearColor(m_MatteColour[0], m_MatteColour[1], m_MatteColour[2], 1.0f);
	// we could also force glDepthRange here, but I preffer relaing on that forcing ztrick 0 worked
	return true;
}

Filming::DRAW_RESULT Filming::doWireframe(GLenum mode)
{
	// Wireframe turned off
	if (m_bInWireframe && movie_wireframe->value == 0)
	{
		glLineWidth(1.0f);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		m_bInWireframe = false;
		return DR_NORMAL;
	}
	
	if (movie_wireframe->value == 0)
		return DR_NORMAL;

	m_bInWireframe = true;

	// Keep the same mode as before
	if (mode == m_iLastMode)
		return DR_NORMAL;

	// Record last mode, but record STRIPS for FANS (since they imply the same
	// things in terms of wireframeness.
	m_iLastMode = (mode == GL_TRIANGLE_FAN ? GL_TRIANGLE_STRIP : mode);

	if (movie_wireframe->value == 1 && mode == GL_QUADS)
		return DR_HIDE;
	
	if (movie_wireframe->value == 1 && mode == GL_POLYGON)
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	else if (movie_wireframe->value == 2 && (mode == GL_TRIANGLE_STRIP || mode == GL_TRIANGLE_FAN))
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	else if (movie_wireframe->value == 3)
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	else
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	pEngfuncs->Cvar_SetValue("gl_clear", 1);
	glLineWidth(movie_wireframesize->value);

	return DR_NORMAL;
}

REGISTER_CMD_FUNC_BEGIN(recordmovie)
{
	if (g_Filming.isFilming())
	{
		pEngfuncs->Con_Printf("Already recording!\n");
		return;
	}

	g_Filming.Start();
}

REGISTER_CMD_FUNC_END(recordmovie)
{
	g_Filming.Stop();
}

REGISTER_CMD_FUNC(recordmovie_start)
{
	CALL_CMD_BEGIN(recordmovie);
}

REGISTER_CMD_FUNC(recordmovie_stop)
{
	CALL_CMD_END(recordmovie);
}

void _mirv_matte_setcolorf(float flRed, float flBlue, float flGreen)
{
	// ensure that the values are within the falid range
	clamp(flRed, 0.0f, 1.0f);
	clamp(flGreen, 0.0f, 1.0f);	
	clamp(flBlue, 0.0f, 1.0f);
	// store matte values.
	g_Filming.setMatteColour(flRed, flGreen, flBlue);
}

// that's not too nice, may be someone can code it more efficient (but still readable please):
// also I think you can retrive it directly as float or even dot it as an cvars
REGISTER_CMD_FUNC(matte_setcolor)
{
	if (pEngfuncs->Cmd_Argc() != 4)
	{
		pEngfuncs->Con_Printf("Useage: " PREFIX "matte_setcolour <red: 0-255> <green: 0-255> <blue: 0-255>\n");
		return;
	}

	float flRed = (float) atoi(pEngfuncs->Cmd_Argv(1)) / 255.0f;
	float flGreen = (float) atoi(pEngfuncs->Cmd_Argv(2)) / 255.0f;
	float flBlue = (float) atoi(pEngfuncs->Cmd_Argv(3)) / 255.0f;

	_mirv_matte_setcolorf(flRed, flBlue, flGreen);
}

REGISTER_CMD_FUNC(matte_setcolorf)
{
	if (pEngfuncs->Cmd_Argc() != 4)
	{
		pEngfuncs->Con_Printf("Useage: " PREFIX "matte_setcolourf <red: 0.0-1.0> <green: 0.0-1.0> <blue: 0.0-1.0>\n");
		return;
	}

	float flRed = (float) atof(pEngfuncs->Cmd_Argv(1));
	float flGreen = (float) atof(pEngfuncs->Cmd_Argv(2));
	float flBlue = (float) atof(pEngfuncs->Cmd_Argv(3));

	_mirv_matte_setcolorf(flRed, flBlue, flGreen);
}