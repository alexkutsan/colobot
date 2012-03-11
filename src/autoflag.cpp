﻿// * This file is part of the COLOBOT source code
// * Copyright (C) 2001-2008, Daniel ROUX & EPSITEC SA, www.epsitec.ch
// *
// * This program is free software: you can redistribute it and/or modify
// * it under the terms of the GNU General Public License as published by
// * the Free Software Foundation, either version 3 of the License, or
// * (at your option) any later version.
// *
// * This program is distributed in the hope that it will be useful,
// * but WITHOUT ANY WARRANTY; without even the implied warranty of
// * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// * GNU General Public License for more details.
// *
// * You should have received a copy of the GNU General Public License
// * along with this program. If not, see  http://www.gnu.org/licenses/.

#define STRICT
#define D3D_OVERLOADS

#include <windows.h>
#include <stdio.h>
#include <d3d.h>

#include "struct.h"
#include "D3DEngine.h"
#include "D3DMath.h"
#include "event.h"
#include "misc.h"
#include "iman.h"
#include "math3d.h"
#include "particule.h"
#include "terrain.h"
#include "camera.h"
#include "object.h"
#include "auto.h"
#include "autoflag.h"



#define ADJUST_ANGLE	FALSE		// TRUE -> ajuste les angles des membres


#if ADJUST_ANGLE
static float g_flag1 = 6.00f;
static float g_flag2 = 0.10f;
static float g_flag3 = 2.00f;
#endif


// Constructeur de l'objet.

CAutoFlag::CAutoFlag(CInstanceManager* iMan, CObject* object)
					 : CAuto(iMan, object)
{
	CAuto::CAuto(iMan, object);

	Init();
}

// Destructeur de l'objet.

CAutoFlag::~CAutoFlag()
{
	CAuto::~CAuto();
}


// D�truit l'objet.

void CAutoFlag::DeleteObject(BOOL bAll)
{
	CAuto::DeleteObject(bAll);
}


// Initialise l'objet.

void CAutoFlag::Init()
{
	D3DVECTOR	wind;
	float		angle;

	m_time = 0.0f;
	m_param = 0;
	m_progress = 0.0f;

	wind = m_terrain->RetWind();
	angle = RotateAngle(wind.x, -wind.z);
	m_object->SetAngleY(0, angle);  // oriente le drapeau dans le vent

	m_strong = Length(wind);
}


// D�but d'une action (1 = secoue).

void CAutoFlag::Start(int param)
{
	if ( m_param == 0 )
	{
		m_param = param;
		m_progress = 0.0f;
	}
}


// Gestion d'un �v�nement.

BOOL CAutoFlag::EventProcess(const Event &event)
{
	float	angle;
	int		i;

	CAuto::EventProcess(event);

#if ADJUST_ANGLE
	if ( event.event == EVENT_KEYDOWN )
	{
		if ( event.param == 'E' )  g_flag1 += 0.1f;
		if ( event.param == 'D' )  g_flag1 -= 0.1f;
		if ( event.param == 'R' )  g_flag2 += 0.1f;
		if ( event.param == 'F' )  g_flag2 -= 0.1f;
		if ( event.param == 'T' )  g_flag3 += 0.1f;
		if ( event.param == 'G' )  g_flag3 -= 0.1f;
	}
#endif

	if ( m_engine->RetPause() )  return TRUE;
	if ( event.event != EVENT_FRAME )  return TRUE;

	if ( m_param == 1 )  // secoue ?
	{
		m_progress += event.rTime*(1.0f/2.0f);
		if ( m_progress < 1.0f )
		{
			angle = sinf(m_progress*PI*8.0f)*0.3f*(1.0f-m_progress);
			m_object->SetAngleX(0, angle);
			angle = sinf(m_progress*PI*4.0f)*0.3f*(1.0f-m_progress);
			m_object->SetAngleZ(0, angle);
		}
		else
		{
			m_object->SetAngleX(0, 0.0f);
			m_object->SetAngleZ(0, 0.0f);
			m_param = 0;
			m_progress = 0.0f;
		}
	}

	if ( m_strong == 0.0f )  return TRUE;  // pas de vent ?

	for ( i=0 ; i<4 ; i++ )
	{
#if ADJUST_ANGLE
		angle = sinf(m_time*g_flag1+i*2.0f)*((i+g_flag3)*g_flag2);
#else
		angle = sinf(m_time*6.0f+i*2.0f)*((i+2.0f)*0.1f);
#endif
		m_object->SetAngleY(1+i, angle);
	}

#if ADJUST_ANGLE
	char s[100];
	sprintf(s, "a=%.2f b=%.2f c=%.2f", g_flag1, g_flag2, g_flag3);
	m_engine->SetInfoText(4, s);
#endif
	return TRUE;
}


// Retourne une erreur li�e � l'�tat de l'automate.

Error CAutoFlag::RetError()
{
	return ERR_OK;
}


