//---------------------------------------------------------------------------

#ifndef UnitCHREditorH
#define UnitCHREditorH
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <ExtCtrls.hpp>
#include <Buttons.hpp>
#include <Menus.hpp>
//---------------------------------------------------------------------------
class TFormCHREditor : public TForm
{
__published:	// IDE-managed Components
	TGroupBox *GroupBox1;
	TPaintBox *PaintBoxChr;
	TGroupBox *GroupBox2;
	TSpeedButton *SpeedButtonHFlip;
	TSpeedButton *SpeedButtonVFlip;
	TSpeedButton *SpeedButtonRotateCCW;
	TSpeedButton *SpeedButtonRotateCW;
	TSpeedButton *CHRInc;
	TSpeedButton *CHRDec;
	TSpeedButton *SpeedButton1Up;
	TSpeedButton *SpeedButton1Down;
	TSpeedButton *SpeedButton1Left;
	TSpeedButton *SpeedButton1Right;
	TLabel *Label1;
	TLabel *Label2;
	TLabel *Label3;
	TLabel *Label4;
	TSpeedButton *SpeedButtonToggleCHR;
	TSpeedButton *SpeedButtonToggleEditor;
	TLabel *Label5;
	TLabel *Label6;
	TSpeedButton *SpeedButtonDoWrap;
	TSpeedButton *ButtonBitmaskLo;
	TSpeedButton *ButtonBitmaskHi;
	TLabel *Label7;
	TSpeedButton *btn2x2mode;
	TSpeedButton *Protect0;
	TSpeedButton *Protect1;
	TSpeedButton *Protect2;
	TSpeedButton *Protect3;
	TLabel *Label8;
	TSpeedButton *btnThick;
	TSpeedButton *btnQuant;
	void __fastcall PaintBoxChrPaint(TObject *Sender);
	void __fastcall PaintBoxChrMouseDown(TObject *Sender, TMouseButton Button,
          TShiftState Shift, int X, int Y);
	void __fastcall PaintBoxChrMouseMove(TObject *Sender, TShiftState Shift, int X,
          int Y);
	void __fastcall FormKeyDown(TObject *Sender, WORD &Key, TShiftState Shift);
	void __fastcall SpeedButtonHFlipClick(TObject *Sender);
	void __fastcall SpeedButtonVFlipClick(TObject *Sender);
	void __fastcall SpeedButtonRotateCCWClick(TObject *Sender);
	void __fastcall SpeedButtonRotateCWClick(TObject *Sender);
	void __fastcall FormCreate(TObject *Sender);
	void __fastcall FormMouseWheel(TObject *Sender, TShiftState Shift,
          int WheelDelta, TPoint &MousePos, bool &Handled);
	void __fastcall PaintBoxChrMouseUp(TObject *Sender, TMouseButton Button,
		  TShiftState Shift, int X, int Y);
	void __fastcall PaintBoxChrClick(TObject *Sender);
	void __fastcall CHRIncClick(TObject *Sender);
	void __fastcall SpeedButton1UpClick(TObject *Sender);
	void __fastcall FormActivate(TObject *Sender);
	void __fastcall FormDeactivate(TObject *Sender);
	void __fastcall SpeedButtonToggleCHRClick(TObject *Sender);
	void __fastcall SpeedButtonToggleEditorClick(TObject *Sender);
	void __fastcall PaintBoxChrMouseLeave(TObject *Sender);
	void __fastcall btn2x2modeClick(TObject *Sender);
	void __fastcall Protect0MouseDown(TObject *Sender, TMouseButton Button,
          TShiftState Shift, int X, int Y);
	void __fastcall Protect1MouseDown(TObject *Sender, TMouseButton Button,
          TShiftState Shift, int X, int Y);
	void __fastcall Protect2MouseDown(TObject *Sender, TMouseButton Button,
          TShiftState Shift, int X, int Y);
	void __fastcall Protect3MouseDown(TObject *Sender, TMouseButton Button,
          TShiftState Shift, int X, int Y);
	void __fastcall Protect0MouseEnter(TObject *Sender);
	void __fastcall Protect0MouseLeave(TObject *Sender);
	void __fastcall ButtonBitmaskLoMouseEnter(TObject *Sender);
	void __fastcall CHRIncMouseEnter(TObject *Sender);
	void __fastcall SpeedButtonDoWrapMouseEnter(TObject *Sender);
	void __fastcall SpeedButtonToggleCHRMouseEnter(TObject *Sender);
	void __fastcall SpeedButton1UpMouseEnter(TObject *Sender);
	void __fastcall SpeedButtonHFlipMouseEnter(TObject *Sender);
	void __fastcall SpeedButtonToggleEditorMouseEnter(TObject *Sender);
	void __fastcall btn2x2modeMouseEnter(TObject *Sender);
	void __fastcall btnThickMouseDown(TObject *Sender, TMouseButton Button,
          TShiftState Shift, int X, int Y);
	void __fastcall btnQuantMouseDown(TObject *Sender, TMouseButton Button,
          TShiftState Shift, int X, int Y);
	void __fastcall btnThickMouseEnter(TObject *Sender);
	void __fastcall btnQuantMouseEnter(TObject *Sender);
	void __fastcall FormShow(TObject *Sender);
	void __fastcall FormPaint(TObject *Sender);
	


private:	// User declarations
public:		// User declarations
	__fastcall TFormCHREditor(TComponent* Owner);
	void __fastcall DrawCHR(int,int,int);
	void __fastcall Draw(bool);
	void __fastcall ScrollLeft(void);
	void __fastcall ScrollHorz(bool);
	void __fastcall ScrollUp(void);
	void __fastcall ScrollDown(void);
	void __fastcall MirrorHorizontal(void);
	void __fastcall MirrorVertical(void);
	void __fastcall Fill(TShiftState Shift,int,int,int);
	void __fastcall TileChange(int,int);
	void __fastcall Flip90(bool);
	void __fastcall Rotate4tiles(bool);
	void __fastcall CHRIncDec(TObject *Sender);
};
//---------------------------------------------------------------------------
extern PACKAGE TFormCHREditor *FormCHREditor;
//---------------------------------------------------------------------------
#endif
