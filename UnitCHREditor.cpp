//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "UnitMain.h"
#include "UnitCHREditor.h"
#include "UnitBrush.h"
#include "UnitState.h"

//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TFormCHREditor *FormCHREditor;

extern int outPalette[];
extern int bgPalCur;
extern unsigned char chrBuf[];
extern unsigned char chrBufFill[];

extern int palActive;
extern int penActive;
extern int tileActive;
extern int bankActive;
extern int palBank;

extern int chrSelectRect;
extern int tileSelRectWdt;
extern int tileSelRectHgt;

//fg
extern int iGlobalAlpha;
extern int tmpContinousIncDecTimer;
extern int continousIncDecDuration;
extern int fillPal;
int prevPal;
extern int blendMode;
extern RECT curMainWinPos;
extern bool prefStartShowCHR;
extern int CHREditorHover;
extern bool cueStats;

extern bool cueCHRdraw;
extern bool cueUpdateNametable;
extern bool cueUpdateTiles;
extern bool cueUpdateMetasprite;

extern int	cntCHRstrip;

extern bool bBrushMask[4];

const int originalFormHgt		=388;
const int originalFormWdt		=286;
const int originalPaintBox		=272;
const int originalGroupBoxWdt	=281;
const int originalGroupBoxHgt	=289;
extern bool bSnapToScreen;

void __fastcall TFormCHREditor::DrawCHR(int xs,int ys,int tile)
{
	int i,j,x,y,pp,col;
	TRect r;

	y=ys;
	int grid= FormMain->CHRpixelgrid1->Checked?15:16;

	pp=tile*16+bankActive;

	if(tile>=0&&tile<256)
	{
		for(i=0;i<8;i++)
		{
			x=xs;

			for(j=0;j<8;j++)
			{
				col=(((chr[pp+i]<<j)&128)>>7)|(((chr[pp+i+8]<<j)&128)>>6);

				PaintBoxChr->Canvas->Brush->Color=TColor(outPalette[bgPal[palBank*16+palActive*4+col]]);

				r.left=x;
				r.top=y;
				r.right=x+grid;
				r.Bottom=y+grid;

				PaintBoxChr->Canvas->FillRect(r);

				x+=16;
			}

			y+=16;
		}
	}
	else
	{
		for(i=0;i<8;i++)
		{
			x=xs;
			for(j=0;j<8;j++)
			{
				PaintBoxChr->Canvas->Brush->Color=GroupBox1->Color;
				r.left=x;
				r.top=y;
				r.right=x+grid;
				r.Bottom=y+grid;
				PaintBoxChr->Canvas->FillRect(r);
				x+=16;
			}
			y+=16;
		}
	}
}



void __fastcall TFormCHREditor::Draw(bool drawAll)
{
	int tx,ty;
	bool a=drawAll;
	bool b=btn2x2mode->Down;
	int n=b?0:8;

	tx=tileActive&15;
	ty=tileActive/16;


	//the checks lessen the burden of drawing these at high rates.

	if(a){
			DrawCHR(-64        			,-64        ,ty>0&&tx>0  ?tileActive-17:-1); //NW
			DrawCHR( 64+8      			,-64        ,ty>0        ?tileActive-16:-1); //N
			DrawCHR( 64+8+128+n			,-64        ,ty>0&&tx<15 ?tileActive-15:-1);
	if(b) 	DrawCHR( 64+8+128+n+128+8	,-64        ,ty>0&&tx<14 ?tileActive-14:-1);
	}

	if(a)		DrawCHR(-64        			, 64+8      ,tx>0        ?tileActive-1 :-1);
				DrawCHR( 64+8      			, 64+8      ,tileActive);
	if(b || a)	DrawCHR( 64+8+128+n			, 64+8      ,tx<15       ?tileActive+1 :-1);
	if(b && a)  DrawCHR( 64+8+128+n+128+8	, 64+8      ,tx<14       ?tileActive+2 :-1);
	//}

	if(b || a)		DrawCHR(-64        			, 64+8+128+n,ty<15&&tx>0 ?tileActive+15:-1);
	if(b || a)		DrawCHR( 64+8      			, 64+8+128+n,ty<15       ?tileActive+16:-1);
	if(b || a)		DrawCHR( 64+8+128+n			, 64+8+128+n,ty<15&&tx<15?tileActive+17:-1);
	if(b)   		DrawCHR( 64+8+128+n+128+8	, 64+8+128+n,ty<15&&tx<14?tileActive+18:-1);

	if(b && a)
	{
			DrawCHR(-64        			, 64+8+128+n+128+8,ty<14&&tx>0 ?tileActive+31:-1);
			DrawCHR( 64+8      			, 64+8+128+n+128+8,ty<14       ?tileActive+32:-1);
			DrawCHR( 64+8+128+n			, 64+8+128+n+128+8,ty<14&&tx<15?tileActive+33:-1);
			DrawCHR( 64+8+128+n+128+8	, 64+8+128+n+128+8,ty<14&&tx<14?tileActive+34:-1);
	}
}



void __fastcall TFormCHREditor::ScrollLeft(void)
{
	/*int i,pp;
	int fOff=0;
	int fLen=16;
	char c[16];   //carry

	if (!ButtonBitmaskLo->Down&&FormMain->Applytonudge1->Checked) {fLen-=8; fOff=-8;}
	if (!ButtonBitmaskHi->Down&&FormMain->Applytonudge1->Checked) {fLen-=8;}

	if(fLen==0)return;

	FormMain->SetUndo();

	pp=tileActive*16+bankActive+fOff;

	for(i=0;i<fLen;i++)
	{
		if(ButtonNudgeInto->Down)
		{
			c[i]=chr[pp];
			chr[pp]=(chr[pp]<<1)|((chr[pp+16]>>7)&1);
			chr[pp+16]=(chr[pp+16]<<1)|((c[i]>>7)&1);
		}
		if(!ButtonNudgeInto->Down) chr[pp]=(chr[pp]<<1)|((chr[pp]>>7)&1);

		pp++;
	}

	Draw(true);

	FormMain->UpdateNameTable(-1,-1,true);
	FormMain->UpdateTiles(false);
	FormMain->UpdateMetaSprite();*/
}



void __fastcall TFormCHREditor::ScrollHorz(bool isLeft)
{
	int i,j,k,l,pp;

	int dir=1;
	int fOff=0;
	int fLen=16;  // first 8 bytes low bit plane, next 8 bytes high bitplane
	char c[16];   //carry

	int xTiles;  //number of tiles to process
	int yTiles;
	//if(chrSelectRect)
	//{
		xTiles=tileSelRectWdt;
		yTiles=tileSelRectHgt;
	//}

	int xb=xTiles*16;
	int yb; //assigned a value within the loop that decides left/right mode.

	if (!ButtonBitmaskHi->Down&&FormMain->Applytonudge1->Checked) {fLen-=8; fOff=-8;}
	if (!ButtonBitmaskLo->Down&&FormMain->Applytonudge1->Checked) {fLen-=8;}

	if(fLen==0||tileSelRectWdt==0||tileSelRectHgt==0)return;
	//^ tileSelRectHgt or Wdt can be 0 if the selection was negative
	FormMain->SetUndo();

	if(isLeft) dir=8*xTiles-1;	//replaces "run once" (rightbound mode) with "run until wrapping to -1" (leftbound mode).



	for(l=0;l<dir;l++)
	{
		yb=yTiles*256;          //yb needs to be refreshed after each iteration for isLeft mode to work.

		for (k=0;k<yTiles;k++)
		{
			yb-=256;
			pp = tileActive*16 + bankActive + fOff + xb + yb;

			// ---------
			// prep carry
			// ---------
			for(i=0;i<fLen;i++)
			{
				pp--;
				c[i]=chr[pp]; //prep carry for the wraparound. we're actually only interested in one bit per byte.
			}
			// ---------
			// shift a row
			// ---------

			pp+=fLen; // restore pp back to its original value

			for (j=0;j<xTiles; j++)
			{

				for(i=0;i<fLen;i++)
				{
					pp--;
					chr[pp]=chr[pp]>>1|((chr[pp-16]<<7)&128);  //move everything one px right and copyborrow from the left tiles' rightmost column

				}
			if (fLen==8) pp-=8;
			}

			// ---------
			// merge carry with first tile
			// ---------
			pp+=16;

			for(i=0;i<fLen;i++)
			{
				pp--;
				chr[pp]=((chr[pp])&127)|((c[i]<<7)&128); //clob together the first tile in a row with the carry from the last tile
			}
		}
	}
	Draw(true);

	FormMain->UpdateNameTable(-1,-1,true);
	FormMain->UpdateTiles(false);
	FormMain->UpdateMetaSprite();
}



void __fastcall TFormCHREditor::ScrollUp(void)
{
	int i,j,k,pp,t1,t2;
	//int tiles=4;
	int xTiles=tileSelRectWdt;
	int yTiles=tileSelRectHgt;
	//int xb=(xTiles-1)*16;

	if (!(ButtonBitmaskLo->Down||ButtonBitmaskHi->Down))
		if(FormMain->Applytonudge1->Checked)  return;
	//if (ButtonNudgeInto->Down)tiles=2;


	FormMain->SetUndo();





	for(k=0;k<xTiles;k++)
	{
		pp=tileActive*16+bankActive+(k*16);
		t1=chr[pp];
		t2=chr[pp+8];

		for (j= 0; j < yTiles; j++)
		{
			for(i=0;i<7;i++)
			{
				if(ButtonBitmaskLo->Down||!FormMain->Applytonudge1->Checked) chr[pp]=chr[pp+1];
				if(ButtonBitmaskHi->Down||!FormMain->Applytonudge1->Checked) chr[pp+8]=chr[pp+9];
				pp++;
			}

			if(yTiles==1||j+1==yTiles)
			{
				if(ButtonBitmaskLo->Down||!FormMain->Applytonudge1->Checked) chr[pp]=t1;
				if(ButtonBitmaskHi->Down||!FormMain->Applytonudge1->Checked) chr[pp+8]=t2;
			}
			else
			{
			   if(ButtonBitmaskLo->Down||!FormMain->Applytonudge1->Checked) chr[pp]=chr[pp+249];
			   if(ButtonBitmaskHi->Down||!FormMain->Applytonudge1->Checked) chr[pp+8]=chr[pp+249+8];
			}

			pp+=249; //derived from 16 bytes *16 tiles - 7 steps

		}
	}
	Draw(true);

	FormMain->UpdateNameTable(-1,-1,true);
	FormMain->UpdateTiles(false);
	FormMain->UpdateMetaSprite();
}



void __fastcall TFormCHREditor::ScrollDown(void)
{
	int i,j,k,pp,off,t1,t2;
	//int tiles=1;
	int xTiles=tileSelRectWdt;
	int yTiles=tileSelRectHgt;

	if (!(ButtonBitmaskLo->Down||ButtonBitmaskHi->Down))
		if(FormMain->Applytonudge1->Checked) return;
	//if (ButtonNudgeInto->Down)tiles=2;
	off=(yTiles-1)*256; // 16 bytes * 16 tiles, not including the first count

	FormMain->SetUndo();

	for(k=0;k<xTiles;k++)
	{
		pp=tileActive*16+off+7+bankActive+k*16;
		t1=chr[pp];
		t2=chr[pp+8];
		for (j= 0; j < yTiles; j++)
		{
			for(i=0;i<7;i++)
			{
				if(ButtonBitmaskLo->Down||!FormMain->Applytonudge1->Checked) chr[pp]=chr[pp-1];
				if(ButtonBitmaskHi->Down||!FormMain->Applytonudge1->Checked) chr[pp+8]=chr[pp+7];
				pp--;
			}

			if(yTiles==1||j+1==yTiles)
			{
				if(ButtonBitmaskLo->Down||!FormMain->Applytonudge1->Checked) chr[pp]=t1;
				if(ButtonBitmaskHi->Down||!FormMain->Applytonudge1->Checked) chr[pp+8]=t2;
			}
			else
			{
				if(ButtonBitmaskLo->Down||!FormMain->Applytonudge1->Checked) chr[pp]=chr[pp-256+8-1];
				if(ButtonBitmaskHi->Down||!FormMain->Applytonudge1->Checked) chr[pp+8]=chr[pp-256+8+7];
			}
			pp-=256-7;
		}
	}
	Draw(true);

	FormMain->UpdateNameTable(-1,-1,true);
	FormMain->UpdateTiles(false);
	FormMain->UpdateMetaSprite();
}



void __fastcall TFormCHREditor::MirrorHorizontal(void)
{
	int i,j,pp,tmp;
    int x,y;
	int xTiles=tileSelRectWdt;
	int yTiles=tileSelRectHgt;

	int fLen=16;
	int fOff=0;

	if (!(ButtonBitmaskLo->Down||ButtonBitmaskHi->Down))
		if(FormMain->Applytomirror1->Checked) return;

	FormMain->SetUndo();

	if (!ButtonBitmaskLo->Down&&FormMain->Applytomirror1->Checked) {fLen-=8; fOff=8;}
	if (!ButtonBitmaskHi->Down&&FormMain->Applytomirror1->Checked) {fLen-=8;}


	for (y = 0; y < yTiles; y++)
	{
		for (x = 0; x < xTiles; x++)
		{
			pp=tileActive*16+bankActive+fOff+y*256+x*16;
			for(i=0;i<fLen;i++)
			{
				tmp=0;

				for(j=0;j<8;j++)
				{
					tmp|=(chr[pp]&(128>>j))?1<<j:0;
				}

				chr[pp++]=tmp;
			}

		}
	}

    bool b=btn2x2mode->Down?1:0;
	if(b && tileSelRectWdt==2 && tileSelRectHgt==2)
	{
		int pp=tileActive*16+bankActive;
		unsigned char tempchr[8];

		if(ButtonBitmaskLo->Down||!FormMain->Applytorotate1->Checked){
			memcpy(tempchr,&chr[pp],8);
			memcpy(&chr[pp],&chr[pp+16],8);
			memcpy(&chr[pp+16],tempchr,8);

			memcpy(tempchr,&chr[pp+16*16],8);
			memcpy(&chr[pp+16*16],&chr[pp+16*16+16],8);
			memcpy(&chr[pp+16*16+16],tempchr,8);

		}

		if(ButtonBitmaskHi->Down||!FormMain->Applytorotate1->Checked){
			pp+=8;
			memcpy(tempchr,&chr[pp],8);
			memcpy(&chr[pp],&chr[pp+16],8);
			memcpy(&chr[pp+16],tempchr,8);

			memcpy(tempchr,&chr[pp+16*16],8);
			memcpy(&chr[pp+16*16],&chr[pp+16*16+16],8);
			memcpy(&chr[pp+16*16+16],tempchr,8);
		}
	}

	Draw(true);

	FormMain->UpdateNameTable(-1,-1,true);
	FormMain->UpdateTiles(false);
	FormMain->UpdateMetaSprite();
}



void __fastcall TFormCHREditor::MirrorVertical(void)
{
	int i,pp;
	int x,y;
	int xTiles=tileSelRectWdt;
	int yTiles=tileSelRectHgt;
	unsigned char tmp[16];

	if (!(ButtonBitmaskLo->Down||ButtonBitmaskHi->Down))
		if(FormMain->Applytomirror1->Checked) return;

	FormMain->SetUndo();

	for (y = 0; y < yTiles; y++)
	{
		for (x = 0; x < xTiles; x++)
		{
			pp=tileActive*16+bankActive+y*256+x*16;
			for(i=0;i<8;i++)
			{
				if(ButtonBitmaskLo->Down||!FormMain->Applytomirror1->Checked) tmp[i]=chr[pp];
				if(ButtonBitmaskHi->Down||!FormMain->Applytomirror1->Checked) tmp[i+8]=chr[pp+8];
				pp++;
			}
			for(i=0;i<8;i++)
			{
				pp--;
				if(ButtonBitmaskLo->Down||!FormMain->Applytomirror1->Checked) chr[pp]=tmp[i];
				if(ButtonBitmaskHi->Down||!FormMain->Applytomirror1->Checked) chr[pp+8]=tmp[i+8];
			}
		}
	}
	bool b=btn2x2mode->Down?1:0;
	if(b && tileSelRectWdt==2 && tileSelRectHgt==2)
	{
		int pp=tileActive*16+bankActive;
		unsigned char tempchr[8];

		if(ButtonBitmaskLo->Down||!FormMain->Applytorotate1->Checked){
			memcpy(tempchr,&chr[pp],8);
			memcpy(&chr[pp],&chr[pp+16*16],8);
			memcpy(&chr[pp+16*16],tempchr,8);

			memcpy(tempchr,&chr[pp+16],8);
			memcpy(&chr[pp+16],&chr[pp+16*16+16],8);
			memcpy(&chr[pp+16*16+16],tempchr,8);

		}

		if(ButtonBitmaskHi->Down||!FormMain->Applytorotate1->Checked){
			pp+=8;
			memcpy(tempchr,&chr[pp],8);
			memcpy(&chr[pp],&chr[pp+16*16],8);
			memcpy(&chr[pp+16*16],tempchr,8);

			memcpy(tempchr,&chr[pp+16],8);
			memcpy(&chr[pp+16],&chr[pp+16*16+16],8);
			memcpy(&chr[pp+16*16+16],tempchr,8);
		}
	}
	Draw(true);

	FormMain->UpdateNameTable(-1,-1,true);
	FormMain->UpdateTiles(false);
	FormMain->UpdateMetaSprite();
}



void __fastcall TFormCHREditor::Flip90(bool dir)
{
	int i,j,pp;
	int x,y;
	int xTiles=tileSelRectWdt;
	int yTiles=tileSelRectHgt;

	unsigned char tile[8][8],tile_flip[8][8];
	if (!(ButtonBitmaskLo->Down||ButtonBitmaskHi->Down))
		if(FormMain->Applytorotate1->Checked)return;
	FormMain->SetUndo();

	for (y = 0; y < yTiles; y++)
	{
		for (x = 0; x < xTiles; x++)
		{
			pp=tileActive*16+bankActive+y*256+x*16;
	for(i=0;i<8;i++)
	{
		for(j=0;j<8;j++)
		{
			tile[i][j]=(chr[pp]&(128>>j)?1:0)|(chr[pp+8]&(128>>j)?2:0);
		}

		pp++;
	}

	for(i=0;i<8;i++)
	{
		for(j=0;j<8;j++)
		{
			tile_flip[i][j]=dir?tile[j][7-i]:tile[7-j][i];
		}
	}

	//pp=tileActive*16+bankActive;
	pp=tileActive*16+bankActive+y*256+x*16;

	for(i=0;i<8;i++)
	{
		chr[pp+0]=0;
		chr[pp+8]=0;

		for(j=0;j<8;j++)
		{
			if(ButtonBitmaskLo->Down||!FormMain->Applytorotate1->Checked) chr[pp+0]|= (tile_flip[i][j]&1)<<(7-j);
			else chr[pp+0]|= (tile[i][j]&1)<<(7-j);
			if(ButtonBitmaskHi->Down||!FormMain->Applytorotate1->Checked) chr[pp+8]|=((tile_flip[i][j]&2)>>1)<<(7-j);
			else chr[pp+8]|=((tile[i][j]&2)>>1)<<(7-j);
		}

		pp++;
	}
	}
		}
}

 void __fastcall TFormCHREditor::Rotate4tiles(bool dir)
 {
	bool b=btn2x2mode->Down?1:0;
	int pp=tileActive*16+bankActive;
	unsigned char tempchr[8];

	if(b && tileSelRectWdt==2 && tileSelRectHgt==2)

	{
		if(!dir){

			if(ButtonBitmaskLo->Down||!FormMain->Applytorotate1->Checked){
				memcpy(tempchr,&chr[pp],8);
				memcpy(&chr[pp],&chr[pp+16*16],8);
				memcpy(&chr[pp+16*16],&chr[(pp+16)+16*16],8);
				memcpy(&chr[(pp+16)+16*16],&chr[pp+16],8);
				memcpy(&chr[pp+16],tempchr,8);
			}

			if(ButtonBitmaskHi->Down||!FormMain->Applytorotate1->Checked){
				pp+=8;
				memcpy(tempchr,&chr[pp],8);
				memcpy(&chr[pp],&chr[pp+16*16],8);
				memcpy(&chr[pp+16*16],&chr[(pp+16)+16*16],8);
				memcpy(&chr[(pp+16)+16*16],&chr[pp+16],8);
				memcpy(&chr[pp+16],tempchr,8);
			}
		}
		else{
        	if(ButtonBitmaskLo->Down||!FormMain->Applytorotate1->Checked){
				memcpy(tempchr,&chr[pp],8);
				memcpy(&chr[pp],&chr[pp+16],8);
				memcpy(&chr[pp+16],&chr[(pp+16)+16*16],8);
				memcpy(&chr[(pp+16)+16*16],&chr[pp+16*16],8);
				memcpy(&chr[pp+16*16],tempchr,8);
			}

			if(ButtonBitmaskHi->Down||!FormMain->Applytorotate1->Checked){
				pp+=8;
				memcpy(tempchr,&chr[pp],8);
				memcpy(&chr[pp],&chr[pp+16],8);
				memcpy(&chr[pp+16],&chr[(pp+16)+16*16],8);
				memcpy(&chr[(pp+16)+16*16],&chr[pp+16*16],8);
				memcpy(&chr[pp+16*16],tempchr,8);
			}
		}
	}

 }

void __fastcall TFormCHREditor::Fill(TShiftState Shift,int x,int y,int extpp)                  //int tempPal, int fillPal,
{
	unsigned char buf[10][10];
	int i,j,pp,col,cnt;


	if(extpp<0) pp=tileActive*16+bankActive;   //typical CHR editor behaviour
	else pp=extpp;                             //typical tileset editor behaviour

	for(i=0;i<10;i++)
	{
		for(j=0;j<10;j++)
		{
			if(i==0||i==9||j==0||j==9)
			{
				buf[i][j]=255;
			}
			else
			{
				if ((penActive==0&&(!Shift.Contains(ssShift)))||extpp>=0)
						buf[i][j]=(chr[pp]&(128>>(j-1))?1:0)|(chr[pp+8]&(128>>(j-1))?2:0);
				else    buf[i][j]=(chrBufFill[pp]&(128>>(j-1))?1:0)|(chrBufFill[pp+8]&(128>>(j-1))?2:0);
			}
		}
		if(i>0&&i<9) pp++;
	}

	col=buf[y+1][x+1];
	if (penActive==0&&(!Shift.Contains(ssShift))) {
		if(col==bgPalCur) return;
	}
	else {} // reserved for new inc/dec fill behaviour.

	buf[y+1][x+1]=254;

	while(true)
	{
		cnt=0;
		for(i=1;i<9;i++)
		{
			for(j=1;j<9;j++)
			{
				if(buf[i][j]==254)
				{
					if(buf[i-1][j]==col) buf[i-1][j]=254;
					if(buf[i+1][j]==col) buf[i+1][j]=254;
					if(buf[i][j-1]==col) buf[i][j-1]=254;
					if(buf[i][j+1]==col) buf[i][j+1]=254;
					buf[i][j]=253;
					cnt++;
				}
			}
		}
		if(!cnt) break;
	}

	if (penActive==0&&(!Shift.Contains(ssShift)))
	{
		for(i=1;i<9;i++)
		{
			for(j=1;j<9;j++)
			{
				if(buf[i][j]==253) buf[i][j]=bgPalCur;
			}
		}
	}
	if (penActive==1||penActive==2||(Shift.Contains(ssShift)))
	{
		if (penActive==1||(Shift.Contains(ssShift)))
		{
			if(!Shift.Contains(ssAlt))fillPal++;
			else fillPal--;
		}
		else //penActive must be 2
		{
			if(!Shift.Contains(ssAlt))fillPal--;
			else fillPal++;
		}

		if(FormMain->IncDecCap1->Checked==true)
		{
			if (fillPal >3) fillPal = 3;
			if (fillPal <0) fillPal = 0;
		}

		for(i=1;i<9;i++)
		{
			for(j=1;j<9;j++)
			{
				if(buf[i][j]==253) buf[i][j]=fillPal;
			}
		}
	}


	if(extpp<0) pp=tileActive*16+bankActive;   //typical CHR editor behaviour
	else pp=extpp;                             //typical tileset editor behaviour

	for(i=1;i<9;i++)
	{
		chr[pp]=0;
		chr[pp+8]=0;
		for(j=1;j<9;j++)
		{
			chr[pp]|=(buf[i][j]&1)<<(8-j);
			chr[pp+8]|=((buf[i][j]&2)>>1)<<(8-j);
		}
		pp++;
	}
}



void __fastcall TFormCHREditor::TileChange(int xoff,int yoff)
{
	int tx,ty;
	bool b=btn2x2mode->Down;
	int n=b?14:15;

	tx=(tileActive&15)+xoff;
	ty=(tileActive/16)+yoff;

	if(tx<0||tx>n||ty<0||ty>n) return;

	FormMain->SetTile(tileActive+=(yoff*16+xoff));
	if(b) {}

	Draw(true);
}



//---------------------------------------------------------------------------
__fastcall TFormCHREditor::TFormCHREditor(TComponent* Owner)
: TForm(Owner)
{
}
//---------------------------------------------------------------------------
void __fastcall TFormCHREditor::PaintBoxChrPaint(TObject *Sender)
{
	Draw(true);
}
//---------------------------------------------------------------------------

void __fastcall TFormCHREditor::PaintBoxChrMouseDown(TObject *Sender,
TMouseButton Button, TShiftState Shift, int X, int Y)
{
	bool b=btn2x2mode->Down?1:0;
	int n=b?128:0;

	//if(!Shift.Contains(ssCtrl)&&!Shift.Contains(ssRight)) FormMain->SetUndo();
	if(!Shift.Contains(ssRight)) FormMain->SetUndo();

	if(Y<64)
	{
		if(X<64) TileChange(-1,-1);
		if(X>=64+8&&X<64+8+128+n) TileChange(0,-1);
		if(X>=64+8+128+8+n) TileChange(1,-1);
	}

	if(Y>=64+8&&Y<64+8+128+n)
	{
		if(X<64) TileChange(-1,0);
		if(X>=64+8+128+8+n) TileChange(1,0);
	}

	if(Y>=64+8+128+8+n)
	{
		if(X<64) TileChange(-1,1);
		if(X>=64+8&&X<64+8+128+n) TileChange(0,1);
		if(X>=64+8+128+8+n) TileChange(1,1);
	}

	if(Y>=64+8 && Y<64+4+128+n && X>=64+8 && X<64+4+128+n)
	{
		memcpy (chrBuf, chr, 8192); //preps a referencepoint for brushstrokes
		memcpy (chrBufFill, chr, 8192); //preps a reference point for inc/dec fill

	}


	PaintBoxChrMouseMove(Sender,Shift,X,Y);
}
//---------------------------------------------------------------------------

void __fastcall TFormCHREditor::PaintBoxChrMouseMove(TObject *Sender,
TShiftState Shift, int X, int Y)
{
	int px,py,pp,mask;
	int tx,ty;
	int ix,iy; //pixel modifiers for thick pen
	int gx,gy,g; //guards for thick pen
	int iThick=btnThick->Down?4:1;
	int tempPal;
	bool b=btn2x2mode->Down?1:0;
	int n=b?128:0;

	CHREditorHover=0; //this can be repurposed later for identifying pixels and memory
					  //for now it just acts to give stats intel.
	cueStats=true;
	
	if(X>=64+8&&X<64+8+128+n&&Y>=64+8&&Y<64+8+128+n)
	{
		if(b){
		//this safeguards drawing outside intended memory when the user manually forces
		//2x2 mode to point towards a down/right edge of the tilesheet
			bool btx=X<64+8+128?false:true;
			bool bty=Y<64+8+128?false:true;

			int ox=(tileActive&15)+btx;
			int oy=(tileActive/16)+bty;

			if(ox>15||oy>15) return;
		}

		tx=X<64+8+128?0:16;
		ty=Y<64+8+128?0:16*16;
		px=((X-(64+8))/16)&7;
		py=((Y-(64+8))/16)&7;
		if(btnQuant->Down){px=px&14; py=py&14;}
		pp=tileActive*16+py+bankActive+tx+ty;
		mask=128>>px;

		if(Shift.Contains(ssLeft))
		{


			 for(int i=0; i<iThick; i++)
			 {

				if(btnThick->Down)
				{
					if(bBrushMask[i]==false) continue;

					if(i==0){ ix=0; iy=0;}
					if(i==1){ ix=16; iy=0;}
					if(i==2){ ix=0; iy=16;}
					if(i==3){ ix=16; iy=16;}
					//recalc pixel to be edited

					tx=X<64+ix+128	?0:16;
					ty=Y<64+iy+128	?0:16*16;

					px=((X-(64+ix))/16)&7;
					py=((Y-(64+iy))/16)&7;

                    gx=((X-(64+ix))/16);
					gy=((Y-(64+iy))/16);

					if(b)g=15; else g=7;
					if(!btnQuant->Down){
						if((i==1||3)&&gx>g) continue;
						if((i==2||3)&&gy>g) continue;
					}
					if(btnQuant->Down){
						px=((X-(64+8))/16)&6;
						py=((Y-(64+8))/16)&6;
						tx=X<64+8+128	?0:16;
						ty=Y<64+8+128	?0:16*16;

						if(i==1){px++;}
						if(i==2){py++;}
						if(i==3){px++; py++;}
					}



					pp=tileActive*16+py+bankActive+tx+ty;
					mask=128>>px;

				}



				tempPal=(((chrBuf[pp]<<px)&128)>>7)|(((chrBuf[pp+8]<<px)&128)>>6);
				fillPal=(((chrBufFill[pp]<<px)&128)>>7)|(((chrBuf[pp+8]<<px)&128)>>6);



				if(FormMain->Applytopen2->Checked){
					if(Protect0->Down && tempPal==0) continue;
					if(Protect1->Down && tempPal==1) continue;
					if(Protect2->Down && tempPal==2) continue;
					if(Protect3->Down && tempPal==3) continue;
				}
				//determine if inc/dec inks should be per click or continous
				if(FormMain->IncDecPerclick1->Checked==false)
				{
					tmpContinousIncDecTimer++;
					if (tmpContinousIncDecTimer>continousIncDecDuration)
					{
						tmpContinousIncDecTimer=0;
						memcpy (chrBuf, chr, 8192);
					}
				}

				if(!Shift.Contains(ssCtrl))
				{
					if (penActive==3&&(!Shift.Contains(ssShift)))
						{
							tmpContinousIncDecTimer++;
							if(tmpContinousIncDecTimer>continousIncDecDuration||tmpContinousIncDecTimer==0)
							{
								tmpContinousIncDecTimer=0;
								if(ButtonBitmaskLo->Down||!FormMain->Applytopen1->Checked) chr[pp]=(chr[pp]&~mask)|(((bgPalCur&1)<<7)>>px);
								if(ButtonBitmaskHi->Down||!FormMain->Applytopen1->Checked) chr[pp+8]=(chr[pp+8]&~mask)|(((bgPalCur&2)<<6)>>px);
							}
					   }
					if (penActive==0&&(!Shift.Contains(ssShift)))
					{

						if(ButtonBitmaskLo->Down||!FormMain->Applytopen1->Checked) chr[pp]=(chr[pp]&~mask)|(((bgPalCur&1)<<7)>>px);
						if(ButtonBitmaskHi->Down||!FormMain->Applytopen1->Checked) chr[pp+8]=(chr[pp+8]&~mask)|(((bgPalCur&2)<<6)>>px);
					}
					if (penActive==1||(Shift.Contains(ssShift)))
					{
						if(!Shift.Contains(ssAlt))tempPal++;
						else tempPal--;

						if(FormMain->IncDecCap1->Checked==true)
						{
							if (tempPal >3) tempPal = 3;
							if (tempPal <0) tempPal = 0;
						}


						if(ButtonBitmaskLo->Down||!FormMain->Applytopen1->Checked) chr[pp]=(chr[pp]&~mask)|(((tempPal&1)<<7)>>px);
						if(ButtonBitmaskHi->Down||!FormMain->Applytopen1->Checked) chr[pp+8]=(chr[pp+8]&~mask)|(((tempPal&2)<<6)>>px);

					}
						if (penActive==2)
					{
						if(!Shift.Contains(ssAlt))tempPal--;
						else tempPal++;

						if(FormMain->IncDecCap1->Checked==true)
						{
							if (tempPal >3) tempPal = 3;
							if (tempPal <0) tempPal = 0;
						}

						if(ButtonBitmaskLo->Down||!FormMain->Applytopen1->Checked) chr[pp]=(chr[pp]&~mask)|(((tempPal&1)<<7)>>px);
						if(ButtonBitmaskHi->Down||!FormMain->Applytopen1->Checked) chr[pp+8]=(chr[pp+8]&~mask)|(((tempPal&2)<<6)>>px);

					}

				}
				else
				{

					if(b)pp=tileActive*16+bankActive+tx+ty; else pp = -1;
					Fill(Shift,px,py,pp); //tempPal,fillPal
				}
			

			}

			cueUpdateMetasprite=true;
			cueUpdateNametable=true;
			cueUpdateTiles=true;
			cueCHRdraw=true;
			//if(b)cueCHRdraw=true;
			
			//if(!b)Draw(false);
			//FormMain->UpdateNameTable(-1,-1,true);
			//FormMain->UpdateTiles(false);
			//FormMain->UpdateMetaSprite();
		}

		if(Shift.Contains(ssRight))
		{
			bgPalCur=(((chr[pp]<<px)&128)>>7)|(((chr[pp+8]<<px)&128)>>6);
			FormMain->DrawPalettes();
		}
	
	}

}
//---------------------------------------------------------------------------

void __fastcall TFormCHREditor::FormKeyDown(TObject *Sender, WORD &Key,
TShiftState Shift)
{

	if(Key==VK_PAUSE)
	{
		FormCHREditor->AlphaBlend^=true;
		if (!Shift.Contains(ssShift))
		{
			FormMain->AlphaBlend^=true;
			//FormManageMetasprites->AlphaBlend^=true;
		}
	}
	bool b;
	if(Key==VK_F1) {FormMain->PageControlEditor->ActivePageIndex=0; b=true;}
	if(Key==VK_F2) {FormMain->PageControlEditor->ActivePageIndex=1; b=true;}
	if(Key==VK_F3) {FormMain->PageControlEditor->ActivePageIndex=2; b=true;}
	if (b) {FormMain->PageControlEditorChange(Sender);}




	if(!Shift.Contains(ssCtrl))
	{
		if(Key==VK_ESCAPE) {
			if( FormBrush->Active) {
				FormBrush->Close();
				if(FormCHREditor->Visible) FormCHREditor->Show();
			}
			else Close();
		}
		if(Key==VK_DELETE) FormMain->CopyCHR(false,true);
		if(Key=='A'||Key==VK_LEFT)  ScrollHorz(true);
		if(Key=='D'||Key==VK_RIGHT) ScrollHorz(false);
		if(Key=='W'||Key==VK_UP)    ScrollUp();
		if(Key=='S'||Key==VK_DOWN)  ScrollDown();
		//if(Key=='H') MirrorHorizontal();
		if(Key=='V') MirrorVertical();    //this is an alias for T
		if(Key=='R') SpeedButtonRotateCWClick(Sender);//{Flip90(false); Rotate4tiles(false);}
		if(Key=='L') SpeedButtonRotateCCWClick(Sender);//{Flip90(true); Rotate4tiles(true);}

		if(Key=='B') btnThick->Down^=true;
		if(Key=='N') btnQuant->Down^=true;
		if(Key=='M') FormBrush->Show();

		if(Key=='I') ButtonBitmaskHi->Down^=true;
		if(Key=='O') ButtonBitmaskLo->Down^=true;
		//if(Key=='N') ButtonNudgeInto->Down^=true;
		if(Key=='P') SpeedButtonDoWrap->Down^=true;
		if(Key=='Q') FormMain->Show();//SpeedButtonToggleEditorClick(Sender);
		if(Key=='X') SpeedButtonToggleCHRClick(Sender);

		if(Key=='E') {btn2x2mode->Down^=true;}

		if(Key==VK_OEM_4||Key==VK_OEM_COMMA) FormMain->SpeedButtonPrevMetaSpriteClick(Sender);// [
		if(Key==VK_OEM_6||Key==VK_OEM_PERIOD) FormMain->SpeedButtonNextMetaSpriteClick(Sender);// ]

		if(Key==VK_NUMPAD7) TileChange(-1,-1);
		if(Key==VK_NUMPAD8) TileChange( 0,-1);
		if(Key==VK_NUMPAD9) TileChange(+1,-1);

		if(Key==VK_NUMPAD4) TileChange(-1,0);
		if(Key==VK_NUMPAD5) FormMain->MCHREditorClick(Sender);
		if(Key==VK_NUMPAD6) TileChange(+1,0);

		if(Key==VK_NUMPAD1) TileChange(-1,+1);
		if(Key==VK_NUMPAD2) TileChange( 0,+1);
		if(Key==VK_NUMPAD3) TileChange(+1,+1);


	}

	if(FormMain->PageControlEditor->ActivePage==FormMain->TabSheetName)
	{
		/*
		if(Key==VK_NUMPAD8) FormMain->MovePaletteCursor(-16);
		if(Key==VK_NUMPAD4) FormMain->MovePaletteCursor(-1);
		if(Key==VK_NUMPAD6) FormMain->MovePaletteCursor(1);
		if(Key==VK_NUMPAD5||Key==VK_NUMPAD2) FormMain->MovePaletteCursor(16);

		if(Key==VK_NUMPAD7){bgPalCur=(bgPalCur-1)&3; FormMain->UpdateAll(); }
		if(Key==VK_NUMPAD9){bgPalCur=(bgPalCur+1)&3; FormMain->UpdateAll(); }
		if(Key==VK_NUMPAD1){palActive=(palActive-1)&3; FormMain->UpdateAll(); }

		if(Key==VK_NUMPAD3) {palActive=(palActive+1)&3; FormMain->UpdateAll(); }
		*/



         //todo: maybe make use comma, period?
		if(Shift.Contains(ssCtrl))
		{
			if(Key==VK_OEM_4) FormMain->ChangeNameTableFrame(-1);// [
			if(Key==VK_OEM_6) FormMain->ChangeNameTableFrame(1);// ]
		}
	}

}
//---------------------------------------------------------------------------

void __fastcall TFormCHREditor::SpeedButtonHFlipClick(TObject *Sender)
{
	MirrorHorizontal();
}
//---------------------------------------------------------------------------

void __fastcall TFormCHREditor::SpeedButtonVFlipClick(TObject *Sender)
{
	MirrorVertical();
}
//---------------------------------------------------------------------------

void __fastcall TFormCHREditor::SpeedButtonRotateCCWClick(TObject *Sender)
{
	Flip90(true);
	Rotate4tiles(true);

	Draw(true);

	FormMain->UpdateNameTable(-1,-1,true);
	FormMain->UpdateTiles(false);
	FormMain->UpdateMetaSprite();
}
//---------------------------------------------------------------------------

void __fastcall TFormCHREditor::SpeedButtonRotateCWClick(TObject *Sender)
{

	Flip90(false);
	Rotate4tiles(false);
	Draw(true);

	FormMain->UpdateNameTable(-1,-1,true);
	FormMain->UpdateTiles(false);
	FormMain->UpdateMetaSprite();
}
//---------------------------------------------------------------------------

void __fastcall TFormCHREditor::FormCreate(TObject *Sender)
{
	DoubleBuffered=true;

	GroupBox1->DoubleBuffered=true;
	if(FormCHREditor->Position==poDesigned)
			{
			FormCHREditor->Left=(Screen->Width+FormMain->Width)/2;
			FormCHREditor->Top=(Screen->Height-FormMain->Height)/2;
			//FormCHREditor->Position=poDesigned;
			//FormCHREditor->Left=(curMainWinPos.left+FormMain->Width);
			//FormCHREditor->Top=(curMainWinPos.top);
			//curMainWinPos
			/*TPoint P;
			P = Mouse->CursorPos;
			FormCHREditor->Left=P.x;
			FormCHREditor->Top=P.y;*/
			}
	if(FormMain->CHReditortoolbartop->Checked){
		GroupBox2->Align=alTop;
		GroupBox1->Align=alBottom;
	}
	if(FormMain->CHReditortoolbarbottom->Checked){
		GroupBox1->Align=alTop;
		GroupBox2->Align=alBottom;
	}
	GroupBox1->Color=TColor(0xCFCFCF);
	FormCHREditor->ScreenSnap=bSnapToScreen;
	if(prefStartShowCHR==true) FormCHREditor->Visible=true;
}
//---------------------------------------------------------------------------

void __fastcall TFormCHREditor::FormMouseWheel(TObject *Sender,
	  TShiftState Shift, int WheelDelta, TPoint &MousePos, bool &Handled)
{
	if(WheelDelta<0)
	{

		if(Shift.Contains(ssShift)||Shift.Contains(ssCtrl))
		{
			if(Shift.Contains(ssShift)) tileActive=(tileActive-1)&255;
			if(Shift.Contains(ssCtrl)) tileActive=(tileActive+16)&255;
			FormMain->SetTile(tileActive);
			Draw(true);
		}
		else { bgPalCur=(bgPalCur-1)&3; FormMain->DrawPalettes(); }
	}
	else
	{
		if(Shift.Contains(ssShift)||Shift.Contains(ssCtrl))
		{
		   if(Shift.Contains(ssShift)) tileActive=(tileActive+1)&255;
		   if(Shift.Contains(ssCtrl)) tileActive=(tileActive-16)&255;
		   FormMain->SetTile(tileActive);
		   Draw(true);
		}
		else { bgPalCur=(bgPalCur+1)&3; FormMain->DrawPalettes();  }
	}

	Handled=true;
}
//---------------------------------------------------------------------------

void __fastcall TFormCHREditor::PaintBoxChrMouseUp(TObject *Sender,
	  TMouseButton Button, TShiftState Shift, int X, int Y)
{
    
	if(Y>=64+8 && Y<64+4+128 && X>=64+8 && X<64+4+128)
	{
	//not in use
	}
}
//---------------------------------------------------------------------------

void __fastcall TFormCHREditor::PaintBoxChrClick(TObject *Sender)
{
	//if(Y>=64+8 && Y<64+4+128 && X>=64+8 && X<64+4+128)
	//{
	//	memcpy (chrBuf, chr, 8192); //preps a referencepoint for brushstrokes
	//}
	
}
//---------------------------------------------------------------------------

void __fastcall TFormCHREditor::CHRIncDec(TObject *Sender)
{
   int i,j,pp,mask;
   int tempPal;
   int x,y;
   int xTiles=tileSelRectWdt;
   int yTiles=tileSelRectHgt;

	FormMain->SetUndo();

	for (y = 0; y < yTiles; y++)
	{
		for (x = 0; x < xTiles; x++)
		{
			pp=tileActive*16+bankActive+y*256+x*16;
			for(i=0;i<8;i++)
			{
				for(j=1;j<9;j++)
				{
					mask=128>>8-j;
					tempPal=(((chr[pp]<<8-j)&128)>>7)|(((chr[pp+8]<<8-j)&128)>>6);

					if(Sender==CHRInc)tempPal++;
					if(Sender==CHRDec)tempPal--;

					if(!SpeedButtonDoWrap->Down)
					{
						if (tempPal >3) tempPal = 3;
						if (tempPal <0) tempPal = 0;
					}

					chr[pp]=(chr[pp]&~mask)|(((tempPal&1)<<7)>>8-j);
					chr[pp+8]=(chr[pp+8]&~mask)|(((tempPal&2)<<6)>>8-j);
				}
			pp++;
			}
		}
	}



	Draw(true);

	FormMain->UpdateNameTable(-1,-1,true);
	FormMain->UpdateTiles(false);
	FormMain->UpdateMetaSprite();
}
//---------------------------------------------------------------------------

//-------------
void __fastcall TFormCHREditor::CHRIncClick(TObject *Sender)
{
  CHRIncDec(Sender);
}
//---------------------------------------------------------------------------



void __fastcall TFormCHREditor::SpeedButton1UpClick(TObject *Sender)
{
	if(Sender==SpeedButton1Left)
		ScrollHorz(true);
	if(Sender==SpeedButton1Right)
		ScrollHorz(false);
	if(Sender==SpeedButton1Up)
		ScrollUp();
	if(Sender==SpeedButton1Down)
		ScrollDown();
}
//---------------------------------------------------------------------------


void __fastcall TFormCHREditor::FormActivate(TObject *Sender)
{
	FormCHREditor->ScreenSnap=bSnapToScreen;
	FormCHREditor->AlphaBlendValue=iGlobalAlpha;
	FormMain->Attributes1->Enabled=false;
	FormMain->SelectedOnly1->Enabled=false;
	FormMain->ApplyTiles1->Enabled=false;
	FormMain->ApplyAttributes1->Enabled=false;
}
//---------------------------------------------------------------------------

void __fastcall TFormCHREditor::FormDeactivate(TObject *Sender)
{
	FormMain->Attributes1->Enabled=true;
	FormMain->SelectedOnly1->Enabled=true;
	FormMain->ApplyTiles1->Enabled=true;
	FormMain->ApplyAttributes1->Enabled=true;
}
//---------------------------------------------------------------------------

void __fastcall TFormCHREditor::SpeedButtonToggleCHRClick(TObject *Sender)
{
	if(FormMain->SpeedButtonChrBank1->Down==true)FormMain->SpeedButtonChrBank2->Down=true;
	else FormMain->SpeedButtonChrBank1->Down=true;

	bankActive=FormMain->SpeedButtonChrBank1->Down?0:4096;
	FormMain->UpdateTiles(true);
	FormMain->UpdateNameTable(-1,-1,true);
	FormMain->UpdateMetaSprite();
}
//---------------------------------------------------------------------------

void __fastcall TFormCHREditor::SpeedButtonToggleEditorClick(TObject *Sender)
{
	FormCHREditor->Show();
}
//---------------------------------------------------------------------------

void __fastcall TFormCHREditor::PaintBoxChrMouseLeave(TObject *Sender)
{
	CHREditorHover=-1;
	cueStats=true;
}
//---------------------------------------------------------------------------

void __fastcall TFormCHREditor::btn2x2modeClick(TObject *Sender)
{
	bool b=btn2x2mode->Down;
	int n=b?128:0;

	FormCHREditor->Width =originalFormWdt+n;
	FormCHREditor->Height=originalFormHgt+n;

	GroupBox1->Width	=originalGroupBoxWdt+n;
	GroupBox1->Height	=originalGroupBoxHgt+n;

	PaintBoxChr->Width  =originalPaintBox+n;
	PaintBoxChr->Height  =originalPaintBox+n;

	if(b)
	{
		if((tileActive&15)==15) tileActive--;
		if(tileActive>=0xF0) tileActive-=16;
	}

	FormMain->SetTile(tileActive);
	Draw(true);
}
//---------------------------------------------------------------------------









void __fastcall TFormCHREditor::Protect0MouseDown(TObject *Sender,
	  TMouseButton Button, TShiftState Shift, int X, int Y)
{
	if(Shift.Contains(ssAlt) && !Shift.Contains(ssCtrl)) //set everyone else
	{
			Protect0->Down=true;    //bc click applies after, this one is inverted
			Protect1->Down=true;
			Protect2->Down=true;
			Protect3->Down=true;
	}
	if(Shift.Contains(ssCtrl) && !Shift.Contains(ssAlt)) //clear everyone else
	{
		if(Protect0->Down && (Protect1->Down||Protect2->Down||Protect3->Down))
		{
			Protect0->Down=false; //force always down, except if others are already up

		}
		Protect1->Down=false;
		Protect2->Down=false;
		Protect3->Down=false;
	}
	if(Shift.Contains(ssShift))
	{

		Protect1->Down=false;
		Protect2->Down=false;
		Protect3->Down=false;
		if(!Protect0->Down)
		{
			Protect0->Down=true; //force always up

		}
	}
}
//---------------------------------------------------------------------------


void __fastcall TFormCHREditor::Protect1MouseDown(TObject *Sender,
      TMouseButton Button, TShiftState Shift, int X, int Y)
{
	if(Shift.Contains(ssAlt) && !Shift.Contains(ssCtrl)) //set everyone else
	{
			Protect0->Down=true;
			Protect1->Down=true;    //bc click applies after, this one is inverted
			Protect2->Down=true;
			Protect3->Down=true;
	}
	if(Shift.Contains(ssCtrl) && !Shift.Contains(ssAlt)) //clear everyone else
	{
		if(Protect1->Down && (Protect0->Down||Protect2->Down||Protect3->Down))
		{
			Protect1->Down=false; //force always down, except if others are already up

		}
		Protect0->Down=false;
		Protect2->Down=false;
		Protect3->Down=false;
	}
	if(Shift.Contains(ssShift))
	{

		Protect0->Down=false;
		Protect2->Down=false;
		Protect3->Down=false;
		if(!Protect1->Down)
		{
			Protect1->Down=true; //force always up

		}
	}
}
//---------------------------------------------------------------------------

void __fastcall TFormCHREditor::Protect2MouseDown(TObject *Sender,
      TMouseButton Button, TShiftState Shift, int X, int Y)
{
	if(Shift.Contains(ssAlt) && !Shift.Contains(ssCtrl)) //set everyone else
	{
			Protect0->Down=true;
			Protect1->Down=true;    //bc click applies after, this one is inverted
			Protect2->Down=true;
			Protect3->Down=true;
	}
	if(Shift.Contains(ssCtrl) && !Shift.Contains(ssAlt)) //clear everyone else
	{
		if(Protect2->Down && (Protect0->Down||Protect1->Down||Protect3->Down))
		{
			Protect2->Down=false; //force always down, except if others are already up

		}
		Protect0->Down=false;
		Protect1->Down=false;
		Protect3->Down=false;
	}
	if(Shift.Contains(ssShift))
	{

		Protect0->Down=false;
		Protect1->Down=false;
		Protect3->Down=false;
		if(!Protect2->Down)
		{
			Protect2->Down=true; //force always up

		}
	}
}
//---------------------------------------------------------------------------

void __fastcall TFormCHREditor::Protect3MouseDown(TObject *Sender,
      TMouseButton Button, TShiftState Shift, int X, int Y)
{
	if(Shift.Contains(ssAlt) && !Shift.Contains(ssCtrl)) //set everyone else
	{
			Protect0->Down=true;
			Protect1->Down=true;    //bc click applies after, this one is inverted
			Protect2->Down=true;
			Protect3->Down=true;
	}
	if(Shift.Contains(ssCtrl) && !Shift.Contains(ssAlt)) //clear everyone else
	{
		if(Protect3->Down && (Protect0->Down||Protect1->Down||Protect2->Down))
		{
			Protect3->Down=false; //force always down, except if others are already up

		}
		Protect0->Down=false;
		Protect1->Down=false;
		Protect2->Down=false;
	}
	if(Shift.Contains(ssShift)) //force all up
	{

		Protect0->Down=false;
		Protect1->Down=false;
		Protect2->Down=false;
		if(!Protect3->Down)
		{
			Protect3->Down=true; //force always up

		}
	}
}
//---------------------------------------------------------------------------

void __fastcall TFormCHREditor::Protect0MouseEnter(TObject *Sender)
{
	int n;
	AnsiString str;
	if(Sender==Protect0) n=0;
	if(Sender==Protect1) n=1;
	if(Sender==Protect2) n=2;
	if(Sender==Protect3) n=3;
	str="Protects pixels of colour "+IntToStr(n)+" from being drawn over."
		+"\n\n[Click] to Toggle.\t\t[Ctrl+click] to mute chosen colour while enabling the others."
		+"\n[Shift + click] to enable all.\t[Alt+click] to 'solo' chosen colour while disabling the others.";
	FormMain->LabelStats->Caption=str;
}
//--------------------------------------------------------------------------
void __fastcall TFormCHREditor::Protect0MouseLeave(TObject *Sender)
{
	FormMain->LabelStats->Caption="---";
}
//---------------------------------------------------------------------------

void __fastcall TFormCHREditor::ButtonBitmaskLoMouseEnter(TObject *Sender)
{
	AnsiString str;
	if(Sender==ButtonBitmaskLo && ButtonBitmaskLo->Down)  str="Disable editing the Low bitplane.";
	if(Sender==ButtonBitmaskLo && !ButtonBitmaskLo->Down) str="Enable editing the Low bitplane.";
	if(Sender==ButtonBitmaskHi && ButtonBitmaskHi->Down)  str="Disable editing the High bitplane.";
	if(Sender==ButtonBitmaskHi && !ButtonBitmaskHi->Down) str="Enable editing the High bitplane.";

	str+="\tSee the menu item [Draw->Bitmask options...] for applications.\n\nNES patterns are stored in 2 bitplanes. It can sometimes be useful to split/merge bitplanes or edit them separately.\nThe low bit represents colour 1. The high; colour 2. Both bits set / clear constitute colour 3 / 0, resp.";
	FormMain->LabelStats->Caption=str;


}
//---------------------------------------------------------------------------

void __fastcall TFormCHREditor::CHRIncMouseEnter(TObject *Sender)
{
	AnsiString str;
	if(Sender==CHRInc) {str="Increment the index of each pixel in the selected tile(s).";
		if(SpeedButtonDoWrap->Down) str+="\nOverflow wraps from colour 3 to 0.";
		else str+="\nOverflow caps to colour 3.";
		str+="\n\nWhen a subpalette is ordered from dark to bright, this action is equivalent to making selected tiles brighter.";
		}
	if(Sender==CHRDec) {str="Decrement the index of each pixel in the selected tile(s).";
		if(SpeedButtonDoWrap->Down) str+="\nUnderflow wraps from colour 0 to 3.";
		else str+="\nUnderflow caps to colour 0.";
		str+="\n\nWhen a subpalette is ordered from dark to bright, this action is equivalent to making selected tiles darker.";
		}
	FormMain->LabelStats->Caption=str;
}
//---------------------------------------------------------------------------


void __fastcall TFormCHREditor::SpeedButtonDoWrapMouseEnter(TObject *Sender)
{
	FormMain->LabelStats->Caption="When down, the value +/- buttons will wrap colour indices around from 0 to 3 or vise versa.\nWhen up, the above buttons cap to values 3 or 0 repsectively.";
}
//---------------------------------------------------------------------------

void __fastcall TFormCHREditor::SpeedButtonToggleCHRMouseEnter(TObject *Sender)
{
	FormMain->LabelStats->Caption="[X] Switches between viewing the main (A) and spare (B) tileset.";
}
//---------------------------------------------------------------------------

void __fastcall TFormCHREditor::SpeedButton1UpMouseEnter(TObject *Sender)
{
	AnsiString str;
	bool b =(FormBrush->Active|FormCHREditor->Active);
	if (b)	str="[WASD] Nudges the pattern of the selected tile(s) ";
	else str="Nudges the pattern of the selected tile(s) ";
	if(Sender==SpeedButton1Up) str+="up.";
	if(Sender==SpeedButton1Down) str+="down.";
	if(Sender==SpeedButton1Left) str+="left.";
	if(Sender==SpeedButton1Right) str+="right.";
	str +="\nWraparound occurs at the seams of the box selection.";
	FormMain->LabelStats->Caption=str;

}
//---------------------------------------------------------------------------

void __fastcall TFormCHREditor::SpeedButtonHFlipMouseEnter(TObject *Sender)
{
	//bool b =(FormBrush->Active|FormCHREditor->Active);
	AnsiString str;
	str="[";
	//if (!b) str+="Shift+";      //nope. 
	if(Sender==SpeedButtonHFlip) str+="H] ";
	if(Sender==SpeedButtonVFlip) str+="V or T] ";
	if(Sender==SpeedButtonRotateCCW) str+="L] ";
	if(Sender==SpeedButtonRotateCW) str+="R] ";


	str+="Flips the pattern of the selected tile(s) ";

	if(Sender==SpeedButtonHFlip) str+="horizontally.";
	if(Sender==SpeedButtonVFlip) str+="vertically.";
	if(Sender==SpeedButtonRotateCCW) str+="counter clockwise.";
	if(Sender==SpeedButtonRotateCW) str+="clockwise.";

	if(btn2x2mode->Down) str +="\nIn 2x2 mode; tiles also swap places so long as 2x2 tiles are box selected.";
	FormMain->LabelStats->Caption=str;
}
//---------------------------------------------------------------------------

void __fastcall TFormCHREditor::SpeedButtonToggleEditorMouseEnter(
      TObject *Sender)
{
	FormMain->LabelStats->Caption="[Q] Toggles focus between CHR Editor and Main Editor.\n\nTip:\tToggling focus makes the respective sets of hotkeys available \n\tfrom each window at the press of the hotkey [Q].";
}
//---------------------------------------------------------------------------

void __fastcall TFormCHREditor::btn2x2modeMouseEnter(TObject *Sender)
{
	bool b =(FormBrush->Active|FormCHREditor->Active);
	if(b)	FormMain->LabelStats->Caption="[E] Toggles between 2x2 and 1 tile edit mode.\n\nTip:\tIn 2x2 mode; The flip/rotate actions also swap tile places so long as 2x2 tiles remain box selected.";
    else	FormMain->LabelStats->Caption="[Shift+E] Toggles between 2x2 and 1 tile edit mode.\n\nTip:\tIn 2x2 mode; The flip/rotate actions also swap tile places so long as 2x2 tiles remain box selected.";

}
//---------------------------------------------------------------------------


void __fastcall TFormCHREditor::btnThickMouseDown(TObject *Sender,
      TMouseButton Button, TShiftState Shift, int X, int Y)
{
	if(Shift.Contains(ssRight)) FormBrush->Show();
	FormMain->TogglePenBrush1->Checked=btnThick->Down;
}
//---------------------------------------------------------------------------



void __fastcall TFormCHREditor::btnQuantMouseDown(TObject *Sender,
      TMouseButton Button, TShiftState Shift, int X, int Y)
{
      FormMain->Quantized1->Checked=btnQuant->Down;
}
//---------------------------------------------------------------------------

void __fastcall TFormCHREditor::btnThickMouseEnter(TObject *Sender)
{
	bool b =(FormBrush->Active|FormCHREditor->Active);
	if (b)	FormMain->LabelStats->Caption="[B] Toggles between Pen (1px) and Brush (masked 2x2) tool on the Left mouse button.\n\nTip:\tRight-click this button or press [M] to open brush shape editor.";
	else 	FormMain->LabelStats->Caption="[Shift+B] Toggles between Pen (1px) and Brush (masked 2x2) tool on the Left mouse button.\n\nTip:\tRight-click this button or press [Shift-F4] to open brush shape editor.";
}
//---------------------------------------------------------------------------

void __fastcall TFormCHREditor::btnQuantMouseEnter(TObject *Sender)
{
	bool b =(FormBrush->Active|FormCHREditor->Active);
	if (b)	FormMain->LabelStats->Caption="[N] Quantizes pen/brush application to be valid only for even coordinates; counting from 0.\n\nTips:\tIn pen mode, using this mode while also nudging the patterns can create a variety of patterns.\n\tIn Brush mode, it's effective with different brush masks [M], but also for drawing 'chunky pixels'.";
	else    FormMain->LabelStats->Caption="[Shift+N] Quantizes pen/brush application to be valid only for even coordinates; counting from 0.\n\nTips:\tIn pen mode, using this mode while also nudging the patterns can create a variety of patterns.\n\tIn Brush mode, it's effective with different brush masks [Shift+F4], but also for drawing 'chunky pixels'.";

}
//---------------------------------------------------------------------------

void __fastcall TFormCHREditor::FormShow(TObject *Sender)
{
	FormCHREditor->AlphaBlendValue=iGlobalAlpha;
}
//---------------------------------------------------------------------------

void __fastcall TFormCHREditor::FormPaint(TObject *Sender)
{
	FormCHREditor->AlphaBlendValue=iGlobalAlpha;	
}
//---------------------------------------------------------------------------


