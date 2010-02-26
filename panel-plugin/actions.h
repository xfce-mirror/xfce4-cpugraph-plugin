#include <cpu.h>

void AssociateCommandChange( GtkEntry *entry, CPUGraph *base );
GdkColor ChangeColor( CPUGraph *base, GdkColor color, GtkWidget *button );
void ChangeColor1( GtkButton *button, CPUGraph *base );
void ChangeColor2( GtkButton *button, CPUGraph *base );
void ChangeColor3( GtkButton *button, CPUGraph *base );
void ChangeColor4( GtkButton *button, CPUGraph *base );
void ColorModeChange( GtkOptionMenu *om, CPUGraph *base );
void DialogResponse( GtkWidget *dlg, int response, CPUGraph *base );
void ApplyChanges( CPUGraph *base );
void FrameChange( GtkToggleButton *button, CPUGraph *base );
void ModeChange( GtkOptionMenu *om, CPUGraph *base );
void WidthChange( GtkSpinButton *sb, CPUGraph *base );
void TimeScaleChange( GtkToggleButton *button, CPUGraph *base );
void UpdateChange( GtkOptionMenu *om, CPUGraph *base );
