//---------------------------------------------------------------------------

#ifndef UnitSetSizeH
#define UnitSetSizeH
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <ComCtrls.hpp>
#include <Buttons.hpp>
//---------------------------------------------------------------------------
class TFormSetSize : public TForm
{
__published:	// IDE-managed Components
	TGroupBox *GroupBox1;
	TButton *ButtonCancel;
	TButton *ButtonOK;
	TEdit *EditWidth;
	TEdit *EditHeight;
	TUpDown *UpDownWidth;
	TUpDown *UpDownHeight;
	TRadioButton *RadioButtonNormal;
	TRadioButton *RadioButtonUser;
	TCheckBox *CheckBoxClear;
	TCheckBox *CheckNullTile;
	TSpeedButton *BtnWdtInc;
	TSpeedButton *BtnHgtInc;
	TSpeedButton *BtnWdtDec;
	TSpeedButton *BtnHgtDec;
	void __fastcall ButtonOKClick(TObject *Sender);
	void __fastcall ButtonCancelClick(TObject *Sender);
	void __fastcall FormShow(TObject *Sender);
	void __fastcall EditWidthKeyPress(TObject *Sender, char &Key);
	void __fastcall EditWidthExit(TObject *Sender);
	void __fastcall RadioButtonNormalClick(TObject *Sender);
	void __fastcall FormCreate(TObject *Sender);
	void __fastcall EditWidthClick(TObject *Sender);
	void __fastcall EditHeightExit(TObject *Sender);
	void __fastcall BtnWdtIncClick(TObject *Sender);
	void __fastcall BtnWdtDecClick(TObject *Sender);
	void __fastcall BtnHgtIncClick(TObject *Sender);
	void __fastcall BtnHgtDecClick(TObject *Sender);
private:	// User declarations
public:		// User declarations
	__fastcall TFormSetSize(TComponent* Owner);

	int NewWidth;
	int NewHeight;
	bool Confirm;
};
//---------------------------------------------------------------------------
extern PACKAGE TFormSetSize *FormSetSize;
//---------------------------------------------------------------------------
#endif
