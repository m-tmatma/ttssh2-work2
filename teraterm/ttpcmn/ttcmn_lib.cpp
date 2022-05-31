/*
 * Copyright (C) 2022- TeraTerm Project
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "tt_res.h"
#include "ttcmn_lib.h"
#include "dlglib.h"
#include "ttcmn_notify.h"

/**
 *	VT Window �̃A�C�R���Ƃ��Z�b�g����
 *
 *	@param[in]	cv			�ݒ肷�� Tera Term �� cv
 *	@param[in]	hInstance	�A�C�R����ێ����Ă��郂�W���[���̃C���X�^���X
 *	@param[in]	IconID		�A�C�R����ID
 *
 *	hInstance = NULL, IconID = 0 �Ƃ���ƁA
 *  �R�}���h���C���Ŏw�肳�ꂽ�A�C�R���A
 *	�w�肪�Ȃ����͕W����VT�A�C�R�����Z�b�g�����
 *
 *	�ʒm�̈�̃A�C�R�����Z�b�g����Ƃ��� NotifySetIconID() ���g�p����
 */
void SetVTIconID(TComVar *cv, HINSTANCE hInstance, WORD IconID)
{
	HINSTANCE icon_inst;
	WORD icon_id;
	TTTSet *ts = cv->ts;

	ts->PluginVTIconInstance = hInstance;
	ts->PluginVTIconID = IconID;

	icon_inst = (ts->PluginVTIconInstance == NULL) ? ts->TeraTermInstance : ts->PluginVTIconInstance;
	icon_id = (ts->PluginVTIconID != 0) ? ts->PluginVTIconID :
	                                      (ts->VTIcon != IdIconDefault) ? ts->VTIcon
	                                                                    : IDI_VT;
	TTSetIcon(icon_inst, cv->HWin, MAKEINTRESOURCEW(icon_id), 0);
}
