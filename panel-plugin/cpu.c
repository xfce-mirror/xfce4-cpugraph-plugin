#include "cpu.h"

guint16 _lerp (double t, guint16 a, guint16 b)
{
        return (guint16)(a + t*(b - a));
}
	

G_MODULE_EXPORT void xfce_control_class_init(ControlClass *cc)
{
	xfce_textdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");
	cc->name = "cpugraph";
	cc->caption = "CPU Graph";
	cc->create_control = (CreateControlFunc) CreateControl;
	cc->attach_callback = AttachCallback;
	cc->free = Kill;
	cc->read_config = ReadSettings;
	cc->write_config = WriteSettings;
	cc->set_size = SetSize;
	cc->create_options = CreateOptions;
	cc->set_orientation = SetOrientation;
}

void Kill (Control *control)
{
	CPUGraph *base = (CPUGraph *)control->data;
	if (base->m_TimeoutID)
		g_source_remove (base->m_TimeoutID);
		
	if (base->m_History)
		g_free (base->m_History);
	
	g_free (base);
}

void ReadSettings (Control *control, xmlNode *node)
{
	xmlChar *value;

	if (node == NULL || node->children == NULL)
		return;

	CPUGraph *base = (CPUGraph *)control->data;

	for (node = node->children; node; node = node->next)
	{
		if (xmlStrEqual (node->name, (const xmlChar *)"cpugraph"))
		{
			if ((value = xmlGetProp (node, (const xmlChar *)"UpdateInterval")))
			{
				base->m_UpdateInterval = atoi ((const char *)value);
				if (base->m_TimeoutID)
					g_source_remove (base->m_TimeoutID);	
				int update = 1000;
				if (base->m_UpdateInterval == 0)
					update = 250;
				else if (base->m_UpdateInterval == 1)
					update = 500;
				else if (base->m_UpdateInterval == 2)
					update = 750;
				else
					update = 1000;
				base->m_TimeoutID = g_timeout_add (update, (GtkFunction) UpdateCPU, base);

				g_free (value);
			}
			if ((value = xmlGetProp (node, (const xmlChar *)"Width")))
			{
				base->m_Width = atoi ((const char *)value);
				SetHistorySize (base, base->m_Width); 
				
				g_free (value);
			}
			if ((value = xmlGetProp (node, (const xmlChar *)"Height")))
			{
				base->m_Height = atoi ((const char *)value);
                		g_free (value);
			}
			if ((value = xmlGetProp (node, (const xmlChar *)"Mode")))
			{
				base->m_Mode = atoi ((const char *)value);
				g_free (value);
			}
			if ((value = xmlGetProp (node, (const xmlChar *)"Frame")))
			{
				base->m_Frame = atoi ((const xmlChar *)value);
				UserSetSize (base);
				g_free (value);
			}
			if ((value = xmlGetProp (node, (const xmlChar *)"ColorMode")))
			{
				base->m_ColorMode = atoi ((const char *)value);
				g_free (value);
			}
			if ((value = xmlGetProp (node, (const xmlChar *)"Foreground1")))
			{
				gdk_color_parse ((const char *)value, &base->m_ForeGround1);
				g_free (value);
			}
			if ((value = xmlGetProp (node, (const xmlChar *)"Foreground2")))
			{
				gdk_color_parse ((const char *)value, &base->m_ForeGround2);
				g_free (value);
			}
			if ((value = xmlGetProp (node, (const xmlChar *)"Background")))
			{
				gdk_color_parse ((const char *)value, &base->m_BackGround);
				g_free (value);
			}
			if ((value = xmlGetProp (node, (const xmlChar *)"Framecolor")))
			{
				gdk_color_parse ((const char *)value, &base->m_FrameColor);
				g_free (value);
			}
			if ((value = xmlGetProp (node, (const xmlChar *)"Foreground3")))
			{
				gdk_color_parse ((const char *)value, &base->m_ForeGround3);
			}
		}
	}
}
void WriteSettings (Control *control, xmlNode *node)
{
	CPUGraph *base = (CPUGraph*)control->data;
	char value[10];
	
	xmlNodePtr root = xmlNewTextChild (node, NULL, (xmlChar *)"cpugraph", NULL);

	g_snprintf (value, 4, "%d", base->m_UpdateInterval);
	xmlSetProp (root, (xmlChar *)"UpdateInterval", (const xmlChar *)value);
	
	g_snprintf (value, 8, "%d", base->m_Width);
	xmlSetProp (root, (xmlChar *)"Width", (const xmlChar *)value);

	g_snprintf (value, 8, "%d", base->m_Height);
	xmlSetProp (root, (xmlChar *)"Height", (const xmlChar *)value);

	g_snprintf (value, 4, "%d", base->m_Mode);
	xmlSetProp (root, (xmlChar *)"Mode", (const xmlChar *)value);

	g_snprintf (value, 2, "%d", base->m_Frame);
	xmlSetProp (root, (xmlChar *)"Frame", (const xmlChar *)value);
	
	g_snprintf (value, 4, "%d", base->m_ColorMode);
	xmlSetProp (root, (xmlChar *)"ColorMode", (const xmlChar *)value);

	g_snprintf (value, 8, "#%02X%02X%02X", base->m_ForeGround1.red >> 8,
					       base->m_ForeGround1.green >> 8,
					       base->m_ForeGround1.blue >> 8);
	xmlSetProp (root, (xmlChar *)"Foreground1", (const xmlChar *)value);
	
        g_snprintf (value, 8, "#%02X%02X%02X", base->m_ForeGround2.red >> 8,
                                               base->m_ForeGround2.green >> 8,
                                               base->m_ForeGround2.blue >> 8);
        xmlSetProp (root, (xmlChar *)"Foreground2", (const xmlChar *)value);

        g_snprintf (value, 8, "#%02X%02X%02X", base->m_BackGround.red >> 8,
                                               base->m_BackGround.green >> 8,
                                               base->m_BackGround.blue >> 8);
        xmlSetProp (root, (xmlChar *)"Background", (const xmlChar *)value);

        g_snprintf (value, 8, "#%02X%02X%02X", base->m_FrameColor.red >> 8,
                                               base->m_FrameColor.green >> 8,
                                               base->m_FrameColor.blue >> 8);
        xmlSetProp (root, (xmlChar *)"Framecolor", (const xmlChar *)value);

        g_snprintf (value, 8, "#%02X%02X%02X", base->m_ForeGround3.red >> 8,
		                               base->m_ForeGround3.green >> 8,
		                               base->m_ForeGround3.blue >> 8);
        xmlSetProp (root, (xmlChar *)"Foreground3", (const xmlChar *)value);
														      
														      
}
CPUGraph *NewCPU ()
{
	CPUGraph *base;
	base = g_new0 (CPUGraph, 1);
			
	base->m_Parent = gtk_event_box_new ();
	gtk_widget_show (base->m_Parent);
	
	base->m_Box = GTK_BOX (gtk_hbox_new (FALSE, 5));
	gtk_container_add (GTK_CONTAINER (base->m_Parent), GTK_WIDGET (base->m_Box));
	gtk_widget_show (GTK_WIDGET (base->m_Box));
	
	base->m_Alignment = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
	gtk_box_pack_start (GTK_BOX (base->m_Box), GTK_WIDGET (base->m_Alignment), FALSE, FALSE, 0);
	gtk_widget_show (base->m_Alignment);

	base->m_DrawArea = gtk_drawing_area_new ();
	gtk_container_add (GTK_CONTAINER (base->m_Alignment), GTK_WIDGET (base->m_DrawArea));
	gtk_widget_show (base->m_DrawArea);

	base->m_Width = icon_size[settings.size];
	base->m_Height = icon_size[settings.size];
	SetHistorySize (base, base->m_Width);
	
	base->m_ForeGround1.red = 0;
	base->m_ForeGround1.green = 65535;
	base->m_ForeGround1.blue = 0;
	
	base->m_ForeGround2.red = 65535;
	base->m_ForeGround2.green = 0;
	base->m_ForeGround2.blue = 0;

	base->m_ForeGround3.red = 0;
	base->m_ForeGround3.green = 0;
	base->m_ForeGround3.blue = 65535;

	base->m_BackGround.red = 65535;
	base->m_BackGround.green = 65535;
	base->m_BackGround.blue = 65535;
	
	base->m_Frame = 0;
	base->m_ColorMode = 0;
	base->m_Mode = 1;

	base->m_Tooltip = gtk_tooltips_new ();
	base->m_UpdateInterval = 3;
	gtk_widget_show_all (base->m_Parent);

	return base;
}
gboolean CreateControl (Control *control)
{
	CPUGraph *base = NewCPU ();
	gtk_container_add (GTK_CONTAINER (control->base), GTK_WIDGET (base->m_Parent));

	int update = 1000;
	if (base->m_UpdateInterval == 0)
		update = 250;
	else if (base->m_UpdateInterval == 1)
		update = 500;
	else if (base->m_UpdateInterval == 2)
		update = 750;
	else
		update = 1000;
		
	base->m_TimeoutID = g_timeout_add (update, (GtkFunction) UpdateCPU, base);
	
	control->data = (gpointer)base;
	control->with_popup = FALSE;
	g_signal_connect (base->m_DrawArea, "expose_event", G_CALLBACK (DrawAreaExposeEvent), control->data);
	gtk_widget_set_size_request (base->m_Parent, -1, -1);
			
	return TRUE;
}
void SetRealGeometry (CPUGraph *base)
{
	if (base->m_Orientation == VERTICAL)
		base->m_RealWidth = icon_size[settings.size];
	else
		base->m_RealHeight = icon_size[settings.size];
}
void SetOrientation (Control *control, int orientation)
{
	CPUGraph *base = (CPUGraph *)control->data;
	base->m_Orientation = orientation;
	SetRealGeometry (base);
	if (base->m_Width > base->m_RealWidth && base->m_Orientation == VERTICAL)
		base->m_Width = base->m_RealWidth;
	if (base->m_Height > base->m_RealHeight && base->m_Orientation == HORIZONTAL)
		base->m_Height = base->m_RealHeight;

	if (base->m_TimeoutID)
		g_source_remove (base->m_TimeoutID);

	gtk_widget_hide (base->m_Parent);
	gtk_container_remove (GTK_CONTAINER (base->m_Parent), GTK_WIDGET (base->m_Box));
	if (base->m_Orientation == HORIZONTAL)
		base->m_Box = GTK_BOX (gtk_hbox_new (FALSE, 5));
	else
		base->m_Box = GTK_BOX (gtk_vbox_new (FALSE, 5));

        gtk_container_add (GTK_CONTAINER (base->m_Parent), GTK_WIDGET (base->m_Box));
	gtk_widget_show (GTK_WIDGET (base->m_Box));
                                                                                
        base->m_Alignment = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
        gtk_box_pack_start (GTK_BOX (base->m_Box), GTK_WIDGET (base->m_Alignment), FALSE, FALSE, 0);
        gtk_widget_show (base->m_Alignment);
	                                                                       
        base->m_DrawArea = gtk_drawing_area_new ();
        gtk_container_add (GTK_CONTAINER (base->m_Alignment), GTK_WIDGET (base->m_DrawArea));
        gtk_widget_show (base->m_DrawArea);
	gtk_widget_show (base->m_Parent);
	g_signal_connect (base->m_DrawArea, "expose_event", G_CALLBACK (DrawAreaExposeEvent), control->data);

	int update=1000;
	if (base->m_UpdateInterval == 0)
		update = 250;
	else if (base->m_UpdateInterval == 1)
		update = 500;
	else if (base->m_UpdateInterval == 2)
		update = 750;
	else
		update = 1000;
	base->m_TimeoutID = g_timeout_add (update,
					  (GtkFunction)UpdateCPU, base);

	UserSetSize (base);
}
void AttachCallback (Control *control, const char *signal, GCallback callback, gpointer data)
{
	CPUGraph *base = (CPUGraph *)control->data;
	g_signal_connect (GTK_WIDGET (base->m_Parent), signal, callback, data);
}
void UpdateTooltip (CPUGraph *base)
{
	char tooltip[32];		
	sprintf (tooltip, "Usage: %d%%", base->m_CPUUsage);			
	gtk_tooltips_set_tip (GTK_TOOLTIPS (base->m_Tooltip), base->m_Parent, tooltip, NULL);
}
void SetSize (Control *control, int size)
{
	CPUGraph *base = (CPUGraph *)control->data;

	SetOrientation (control, settings.orientation);
}
void UserSetSize (CPUGraph *base)
{
	int toadd = base->m_Frame ? 2 : 0;
	gtk_widget_set_size_request (base->m_Parent, base->m_Width + toadd, base->m_Height + toadd);
	gtk_widget_set_size_request (base->m_DrawArea, base->m_Width + toadd, base->m_Height + toadd);
	gtk_widget_queue_resize (GTK_WIDGET (base->m_DrawArea));
}
void CreateOptions (Control *control, GtkContainer *container, GtkWidget *done)
{
	CPUGraph *base = (CPUGraph *)control->data;
	GtkBox *vbox, *vbox2, *hbox;
	GtkWidget *label;
	GtkSizeGroup *sg = base->m_Sg;
	SOptions *op = &base->m_Options;
	
	sg = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

	base->m_OptionsDialog = gtk_widget_get_toplevel (done);
	   
	vbox = GTK_BOX (gtk_vbox_new (FALSE, 5));
	gtk_widget_show (GTK_WIDGET (vbox));
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 8);
   
        /* Update Interval */

	hbox = GTK_BOX (gtk_hbox_new (FALSE, 5));
        gtk_widget_show (GTK_WIDGET (hbox));
        gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (hbox), FALSE, FALSE, 0);
        label = gtk_label_new (_("Update Interval: "));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
	gtk_size_group_add_widget (sg, label);
	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (label), FALSE, FALSE, 0);
                  
        op->m_UpdateOption = gtk_option_menu_new ();
        gtk_widget_show (op->m_UpdateOption);
        gtk_box_pack_start (GTK_BOX (hbox), op->m_UpdateOption, FALSE, FALSE, 0);
                                                                                                                           
        op->m_UpdateMenu = gtk_menu_new ();
        gtk_option_menu_set_menu (GTK_OPTION_MENU (op->m_UpdateOption), op->m_UpdateMenu);
                                                                                     
        op->m_UpdateMenuItem = gtk_menu_item_new_with_label (_("Fastest (~250ms)"));
        gtk_widget_show (op->m_UpdateMenuItem);
        gtk_menu_shell_append (GTK_MENU_SHELL (op->m_UpdateMenu), op->m_UpdateMenuItem);
                                                                                                                                                                
        op->m_UpdateMenuItem = gtk_menu_item_new_with_label (_("Fast (~500ms)"));
        gtk_widget_show (op->m_UpdateMenuItem);
        gtk_menu_shell_append (GTK_MENU_SHELL (op->m_UpdateMenu), op->m_UpdateMenuItem);
                                                                                                         
        op->m_UpdateMenuItem = gtk_menu_item_new_with_label (_("Normal (~750ms)"));
        gtk_widget_show (op->m_UpdateMenuItem);
        gtk_menu_shell_append (GTK_MENU_SHELL (op->m_UpdateMenu), op->m_UpdateMenuItem);

	op->m_UpdateMenuItem = gtk_menu_item_new_with_label (_("Slow (~1s)"));
	gtk_widget_show (op->m_UpdateMenuItem);
	gtk_menu_shell_append (GTK_MENU_SHELL (op->m_UpdateMenu), op->m_UpdateMenuItem);

        gtk_option_menu_set_history (GTK_OPTION_MENU (op->m_UpdateOption), base->m_UpdateInterval);
                                                                                                                               
        g_signal_connect (op->m_UpdateOption, "changed", G_CALLBACK (UpdateChange), base);   
        /* Width */

	hbox = GTK_BOX (gtk_hbox_new (FALSE, 5));
	gtk_widget_show (GTK_WIDGET (hbox));
	gtk_box_pack_start (GTK_BOX (vbox),
			    GTK_WIDGET (hbox), FALSE, FALSE, 0);

	label = gtk_label_new (_("Width: "));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
	gtk_size_group_add_widget (sg, label);
	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (label), FALSE, FALSE, 0);

	op->m_Width = gtk_spin_button_new_with_range (4, (base->m_Orientation == HORIZONTAL) ? 128 : base->m_RealWidth, 1);
	base->m_TmpWidth = base->m_Width;
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (op->m_Width), base->m_Width);
	gtk_widget_show (op->m_Width);
	gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (op->m_Width), FALSE, FALSE, 0);
	g_signal_connect (op->m_Width, "value-changed", G_CALLBACK (SpinChange), &base->m_TmpWidth);

	/* Height */
	hbox = GTK_BOX (gtk_hbox_new (FALSE, 5));
	gtk_widget_show (GTK_WIDGET (hbox));
	gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (hbox), FALSE, FALSE, 0);

	label = gtk_label_new (_("Height: "));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
	gtk_size_group_add_widget (sg, label);
	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (label), FALSE, FALSE, 0);

	op->m_Height = gtk_spin_button_new_with_range (4, (base->m_Orientation == HORIZONTAL) ? base->m_RealHeight : 128, 1);
	base->m_TmpHeight = base->m_Height;
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (op->m_Height), base->m_Height);
	gtk_widget_show (op->m_Height);
	gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (op->m_Height), FALSE, FALSE, 0);
	g_signal_connect (op->m_Height, "value-changed", G_CALLBACK (SpinChange), &base->m_TmpHeight);

	/* Frame */
	hbox = GTK_BOX (gtk_hbox_new (FALSE, 5));
	gtk_widget_show (GTK_WIDGET (hbox));
	gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (hbox), FALSE, FALSE, 0);

	op->m_GraphFrame = gtk_check_button_new_with_mnemonic (_("Use frame"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (op->m_GraphFrame), base->m_Frame);
	gtk_widget_show (op->m_GraphFrame);
	gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (op->m_GraphFrame), FALSE, FALSE, 0);
	g_signal_connect (op->m_GraphFrame, "toggled", G_CALLBACK (FrameChange), base);
	gtk_size_group_add_widget (sg, op->m_GraphFrame);

   	/* Frame color */

	op->m_FC = gtk_button_new ();
	op->m_ColorDA4 = gtk_drawing_area_new ();

	gtk_widget_modify_bg (op->m_ColorDA4, GTK_STATE_NORMAL, &base->m_FrameColor);
	gtk_widget_set_size_request (op->m_ColorDA4, 12, 12);
	gtk_container_add (GTK_CONTAINER (op->m_FC), op->m_ColorDA4);
	gtk_widget_show (GTK_WIDGET (op->m_FC));
	gtk_widget_show (GTK_WIDGET (op->m_ColorDA4));
	gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (op->m_FC), FALSE, FALSE, 0);
	gtk_widget_set_sensitive (GTK_WIDGET (base->m_Options.m_FC), base->m_Frame);
	
	g_signal_connect (op->m_FC, "clicked", G_CALLBACK (ChangeColor4), base);
   
        vbox2 = GTK_BOX (gtk_vbox_new (FALSE, 5));
        gtk_widget_show (GTK_WIDGET (vbox2));
        gtk_container_set_border_width (GTK_CONTAINER (vbox2), 8);

	/* Foreground 1 */
	
	hbox = GTK_BOX (gtk_hbox_new (FALSE, 5));
	gtk_widget_show (GTK_WIDGET (hbox));
	gtk_box_pack_start (GTK_BOX (vbox2), GTK_WIDGET (hbox), FALSE, FALSE, 0);

	label = gtk_label_new (_("Color 1: "));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
	gtk_size_group_add_widget (sg, label);
	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (label), FALSE, FALSE, 0);
   
	op->m_FG1 = gtk_button_new ();
	op->m_ColorDA = gtk_drawing_area_new ();

	gtk_widget_modify_bg(op->m_ColorDA, GTK_STATE_NORMAL, &base->m_ForeGround1);
	gtk_widget_set_size_request (op->m_ColorDA, 12, 12);
	gtk_container_add (GTK_CONTAINER (op->m_FG1), op->m_ColorDA);
	gtk_widget_show (GTK_WIDGET (op->m_FG1));
	gtk_widget_show (GTK_WIDGET (op->m_ColorDA));
	gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (op->m_FG1), FALSE, FALSE, 0);

	g_signal_connect (op->m_FG1, "clicked", G_CALLBACK (ChangeColor1), base);

	/* Foreground2 */

	hbox = GTK_BOX (gtk_hbox_new (FALSE, 5));
	gtk_widget_show (GTK_WIDGET (hbox));
	gtk_box_pack_start (GTK_BOX (vbox2), GTK_WIDGET (hbox), FALSE, FALSE, 0);

	label = gtk_label_new (_("Color 2: "));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
	gtk_size_group_add_widget (sg, label);
	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (label), FALSE, FALSE, 0);

	op->m_FG2 = gtk_button_new ();
	op->m_ColorDA2 = gtk_drawing_area_new ();

	gtk_widget_modify_bg(op->m_ColorDA2, GTK_STATE_NORMAL, &base->m_ForeGround2);
	gtk_widget_set_size_request (op->m_ColorDA2, 12, 12);
	gtk_container_add (GTK_CONTAINER (op->m_FG2), op->m_ColorDA2);
	gtk_widget_show (GTK_WIDGET (op->m_FG2));
	gtk_widget_show (GTK_WIDGET (op->m_ColorDA2));
	gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (op->m_FG2), FALSE, FALSE, 0);

	g_signal_connect (op->m_FG2, "clicked", G_CALLBACK (ChangeColor2), base);

	if (base->m_Mode == 1)
		gtk_widget_set_sensitive (GTK_WIDGET (base->m_Options.m_FG2), TRUE);

	/* Foreground3 */
	
	hbox = GTK_BOX (gtk_hbox_new (FALSE, 5));
        gtk_widget_show (GTK_WIDGET (hbox));
        gtk_box_pack_start (GTK_BOX (vbox2), GTK_WIDGET (hbox), FALSE, FALSE, 0);
                                                                                                                                                                          
        label = gtk_label_new (_("Color 3: "));
        gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
        gtk_size_group_add_widget (sg, label);
        gtk_widget_show (label);
        gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (label), FALSE, FALSE, 0);
	                                                                                                                                                                          op->m_FG3 = gtk_button_new ();
        op->m_ColorDA5 = gtk_drawing_area_new ();
        gtk_widget_modify_bg(op->m_ColorDA5, GTK_STATE_NORMAL, &base->m_ForeGround3);
        gtk_widget_set_size_request (op->m_ColorDA5, 12, 12);
        gtk_container_add (GTK_CONTAINER (op->m_FG3), op->m_ColorDA5);
        gtk_widget_show (GTK_WIDGET (op->m_FG3));
        gtk_widget_show (GTK_WIDGET (op->m_ColorDA5));
        gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (op->m_FG3), FALSE, FALSE, 0);
        g_signal_connect (op->m_FG3, "clicked", G_CALLBACK (ChangeColor5), base);
	
        if (base->m_Mode == 0 || base->m_Mode == 2 || base->m_ColorMode == 0)
	        gtk_widget_set_sensitive (GTK_WIDGET (base->m_Options.m_FG3), FALSE);
	else
		gtk_widget_set_sensitive (GTK_WIDGET (base->m_Options.m_FG3), TRUE);
																		
	
	/* Background */

	hbox = GTK_BOX (gtk_hbox_new (FALSE, 5));
	gtk_widget_show (GTK_WIDGET (hbox));
	gtk_box_pack_start (GTK_BOX (vbox2), GTK_WIDGET (hbox), FALSE, FALSE, 0);

	label = gtk_label_new (_("Background: "));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
	gtk_size_group_add_widget (sg, label);
	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (label), FALSE, FALSE, 0);

	op->m_BG = gtk_button_new ();
	op->m_ColorDA3 = gtk_drawing_area_new ();

	gtk_widget_modify_bg(op->m_ColorDA3, GTK_STATE_NORMAL, &base->m_BackGround);
	gtk_widget_set_size_request (op->m_ColorDA3, 12, 12);
	gtk_container_add (GTK_CONTAINER (op->m_BG), op->m_ColorDA3);
	gtk_widget_show (GTK_WIDGET (op->m_BG));
	gtk_widget_show (GTK_WIDGET (op->m_ColorDA3));
	gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (op->m_BG), FALSE, FALSE, 0);

	g_signal_connect (op->m_BG, "clicked", G_CALLBACK (ChangeColor3), base);

	/* Modes */

	hbox = GTK_BOX (gtk_hbox_new (FALSE, 5));
	gtk_widget_show (GTK_WIDGET (hbox));
	gtk_box_pack_start (GTK_BOX (vbox2), GTK_WIDGET (hbox), FALSE, FALSE, 0);

	label = gtk_label_new (_("Mode: "));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
	gtk_size_group_add_widget (sg, label);
	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (label), FALSE, FALSE, 0);

	op->m_OptionMenu = gtk_option_menu_new ();
	gtk_widget_show (op->m_OptionMenu);
	gtk_box_pack_start (GTK_BOX (hbox), op->m_OptionMenu, FALSE, FALSE, 0);

	op->m_Menu = gtk_menu_new ();
	gtk_option_menu_set_menu (GTK_OPTION_MENU (op->m_OptionMenu), op->m_Menu);

	op->m_MenuItem = gtk_menu_item_new_with_label (_("Normal"));
	gtk_widget_show (op->m_MenuItem);
	gtk_menu_shell_append (GTK_MENU_SHELL (op->m_Menu), op->m_MenuItem);
	
	op->m_MenuItem = gtk_menu_item_new_with_label (_("LED"));
	gtk_widget_show (op->m_MenuItem);
	gtk_menu_shell_append (GTK_MENU_SHELL (op->m_Menu), op->m_MenuItem);

	op->m_MenuItem = gtk_menu_item_new_with_label (_("No history"));
	gtk_widget_show (op->m_MenuItem);
	gtk_menu_shell_append (GTK_MENU_SHELL (op->m_Menu), op->m_MenuItem);

	gtk_option_menu_set_history (GTK_OPTION_MENU (op->m_OptionMenu), base->m_Mode);

	g_signal_connect (op->m_OptionMenu, "changed", G_CALLBACK (ModeChange), base);

	/* Color mode */

	hbox = GTK_BOX (gtk_hbox_new (FALSE, 5));
        gtk_widget_show (GTK_WIDGET (hbox));
        gtk_box_pack_start (GTK_BOX (vbox2), GTK_WIDGET (hbox), FALSE, FALSE, 0);
        label = gtk_label_new (_("Color mode: "));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
	gtk_size_group_add_widget (sg, label);
	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (label), FALSE, FALSE, 0);
                  
        op->m_ModeOption = gtk_option_menu_new ();
        gtk_widget_show (op->m_ModeOption);
        gtk_box_pack_start (GTK_BOX (hbox), op->m_ModeOption, FALSE, FALSE, 0);
                                                                                                                           
        op->m_ModeMenu = gtk_menu_new ();
        gtk_option_menu_set_menu (GTK_OPTION_MENU (op->m_ModeOption), op->m_ModeMenu);
                                                                                     
        op->m_ModeMenuItem = gtk_menu_item_new_with_label (_("None"));
        gtk_widget_show (op->m_ModeMenuItem);
        gtk_menu_shell_append (GTK_MENU_SHELL (op->m_ModeMenu), op->m_ModeMenuItem);
                                                                                                                                                                
        op->m_ModeMenuItem = gtk_menu_item_new_with_label (_("Gradient"));
        gtk_widget_show (op->m_ModeMenuItem);
        gtk_menu_shell_append (GTK_MENU_SHELL (op->m_ModeMenu), op->m_ModeMenuItem);
                                                                                                         
        op->m_ModeMenuItem = gtk_menu_item_new_with_label (_("Fire"));
        gtk_widget_show (op->m_ModeMenuItem);
        gtk_menu_shell_append (GTK_MENU_SHELL (op->m_ModeMenu), op->m_ModeMenuItem);

        gtk_option_menu_set_history (GTK_OPTION_MENU (op->m_ModeOption), base->m_ColorMode);
                                                                                                                               
        g_signal_connect (op->m_ModeOption, "changed", G_CALLBACK (ColorModeChange), base);
	
                                                                                                                 
	/* Done */

	g_signal_connect_swapped (done, "clicked", G_CALLBACK (ApplyChanges), base);
			
	gtk_widget_show_all (GTK_WIDGET (hbox));

	op->m_Notebook = gtk_notebook_new ();
        label = gtk_label_new (_("Apperance"));
	gtk_notebook_append_page (GTK_NOTEBOOK (op->m_Notebook), GTK_WIDGET (vbox2), GTK_WIDGET (label));
	label = gtk_label_new (_("Advanced"));
	gtk_notebook_append_page (GTK_NOTEBOOK (op->m_Notebook), GTK_WIDGET (vbox), GTK_WIDGET (label));
	gtk_widget_show (op->m_Notebook);

	gtk_container_add (container, GTK_WIDGET (op->m_Notebook));
	
}
gboolean UpdateCPU (CPUGraph *base)
{
	base->m_CPUUsage = GetCPUUsage (&base->m_OldUsage, &base->m_OldTotal);

	memmove (base->m_History+1, base->m_History, (base->m_Values-1)*sizeof (int));
	base->m_History[0] = base->m_CPUUsage;

	/* Tooltip */
	UpdateTooltip (base);
	
	/* Draw the graph. */
	gtk_widget_queue_draw (base->m_DrawArea);

	return TRUE;
}

void DrawGraph (CPUGraph *base)
{
        GdkGC *fg1, *fg2, *bg, *fc;
	GtkWidget *da = base->m_DrawArea;

	fg1 = gdk_gc_new (da->window);
	gdk_gc_set_rgb_fg_color (fg1, &base->m_ForeGround1);

	fg2 = gdk_gc_new (da->window);
	gdk_gc_set_rgb_fg_color (fg2, &base->m_ForeGround2);

	bg = gdk_gc_new (da->window);
        gdk_gc_set_rgb_fg_color (bg, &base->m_BackGround);

	if (base->m_Frame)
	{
		fc = gdk_gc_new (da->window);
		gdk_gc_set_rgb_fg_color (fc, &base->m_FrameColor);
	}

	int startx = base->m_Frame ? 1 : 0;
	int starty = base->m_Frame ? 1 : 0;

	gdk_draw_rectangle (da->window,
			    bg,
			    TRUE,
			    startx, starty,
			    base->m_Width, base->m_Height);


        if (base->m_Frame)
        {
                gdk_draw_rectangle (da->window,
                                    fc,
                                    FALSE,
                                    0, 0,
                                    base->m_Width+1, base->m_Height+1);
        }
																							

	float step = base->m_Height/100.0;

	if (base->m_Mode == 0)
        {
		int x, y;
		GdkGC *gc;
		if (base->m_ColorMode > 0)
			gc = gdk_gc_new (da->window);
		for (x=base->m_Width - 1;x >= 0;x--)
        	{
			int usage = base->m_History[base->m_Width - 1 - x]*step;
			int tmp = 0;
			int length = base->m_Height - (base->m_Height - usage);
			for (y=base->m_Height-1;y >= base->m_Height - usage;y--)
			{
				if (base->m_ColorMode > 0)
				{
					GdkColor color;
					double t = (base->m_ColorMode == 1) ? (tmp / (double)(base->m_Height-1)) : (tmp / (double)(length-1));
					color.red = _lerp (	t,
								base->m_ForeGround1.red,
								base->m_ForeGround2.red);
					color.green = _lerp (	t,
								base->m_ForeGround1.green,
								base->m_ForeGround2.green);
					color.blue = _lerp (	t,
								base->m_ForeGround1.blue,
								base->m_ForeGround2.blue);
					gdk_gc_set_rgb_fg_color (gc, &color);
					tmp++;
				}
				gdk_draw_point (da->window, (base->m_ColorMode > 0) ? gc : fg1, base->m_Frame ? x+1 : x, base->m_Frame ? y+1 : y);
			}
		}
		if (base->m_ColorMode > 0)
			g_object_unref (gc);
	}
	else if (base->m_Mode == 1)
	{
                GdkGC *gc;
		if (base->m_ColorMode > 0)
			gc = gdk_gc_new (da->window);
                int nrx = base->m_Width/3.0;
                int nry = base->m_Height/2.0;
                float tstep = nry/100.0;
                int x, y;
                for (x=nrx-1;x>=0;x--)
                {
	                int usage = (int)(base->m_History[nrx - 1 - x]*tstep);
			int tmp=0;
			int length = nry - (nry - usage);
                        for (y=nry-1;y>=0;y--)
                        {
				GdkGC *draw = fg2;
				if (base->m_ColorMode > 0)
				{
					GdkColor color;
					double t = (base->m_ColorMode == 1) ? (tmp / (double)(nry-1)) : (tmp / (double)(length-1));
					color.red = _lerp (	t,
								base->m_ForeGround2.red,
								base->m_ForeGround3.red);
					color.green = _lerp (	t,
								base->m_ForeGround2.green,
								base->m_ForeGround3.green);
					color.blue = _lerp (	t,
								base->m_ForeGround2.blue,
								base->m_ForeGround3.blue);
					gdk_gc_set_rgb_fg_color (gc, &color);
					tmp++;
					draw = gc;
				}
				
	                        gdk_draw_rectangle (    da->window,
	                                                ((nry - usage) > y) ? fg1 : draw,
	                                                TRUE,
	                                                base->m_Frame ? x*3+1 : x*3, base->m_Frame ? y*2+1 : y*2,
	                                                2, 1);
	                }
	        }
	}
	else if (base->m_Mode == 2)
	{
		GdkGC *gc;
		if (base->m_ColorMode > 0)
			gc = gdk_gc_new (da->window);
		int y;
		int usage = (int)(base->m_History[0]*step);
		int tmp=0;
		int length = base->m_Height - (base->m_Height - usage);
		for (y=base->m_Height-1;y>=base->m_Height - usage;y--)
		{
			if (base->m_ColorMode > 0)
			{
				GdkColor color;
				double t = (base->m_ColorMode == 1) ? (tmp / (double)(base->m_Height-1)) : (tmp / (double)(length-1));
				color.red = _lerp (	t,
							base->m_ForeGround1.red,
							base->m_ForeGround2.red);
				color.green = _lerp (	t,
							base->m_ForeGround1.green,
							base->m_ForeGround2.green);
				color.blue = _lerp (	t,
							base->m_ForeGround1.blue,
							base->m_ForeGround2.blue);
				gdk_gc_set_rgb_fg_color (gc, &color);
				tmp++;
			}
			gdk_draw_line (	da->window, 
					(base->m_ColorMode > 0) ? gc : fg1,
					base->m_Frame ? 1 : 0, base->m_Frame ? y+1 : y,
					base->m_Width, base->m_Frame ? y+1 : y);
		}
	}
	else if (base->m_Mode == 4)
	{
		gdk_draw_rectangle (da->window,
				    fg1,
				    TRUE,
				    base->m_Frame ? 1 : 0, base->m_Frame ? (base->m_Height - (int)(base->m_History[0]*step))+1 : (base->m_Height-(int)(base->m_History[0]*step)),
				    base->m_Width, (int)(base->m_History[0]*step));
	}
	
	g_object_unref (fg2);
        g_object_unref (fg1);
	g_object_unref (bg);
	if (base->m_Frame)
		g_object_unref (fc);

}
void DrawAreaExposeEvent (GtkWidget *da, GdkEventExpose *event, gpointer data)
{
	CPUGraph *base = (CPUGraph *)data;
	DrawGraph (base);
}

void SpinChange (GtkSpinButton *sb, int *value)
{
	(*value) = gtk_spin_button_get_value_as_int (sb);
}

void ApplyChanges (CPUGraph *base)
{
	if (base->m_TimeoutID)
		g_source_remove (base->m_TimeoutID);	
	int update=1000;
	if (base->m_UpdateInterval == 0)
		update = 250;
	else if (base->m_UpdateInterval == 1)
		update = 500;
	else if (base->m_UpdateInterval == 2)
		update = 750;
	else
		update = 1000;
	base->m_TimeoutID = g_timeout_add (update, (GtkFunction) UpdateCPU, base);

	base->m_Width = base->m_TmpWidth;
	base->m_Height = base->m_TmpHeight;

	UserSetSize (base);
	SetHistorySize (base, base->m_Width);
}

void ChangeColor1 (GtkButton *button, CPUGraph *base)
{
	ChangeColor (0, base);
}
void ChangeColor2 (GtkButton *button, CPUGraph *base)
{
	ChangeColor (1, base);
}
void ChangeColor3 (GtkButton *button, CPUGraph *base)
{
	ChangeColor (2, base);
}
void ChangeColor4 (GtkButton *button, CPUGraph *base)
{
	ChangeColor (3, base);
}
void ChangeColor5 (GtkButton *button, CPUGraph *base)
{
	ChangeColor (4, base);
}
void ChangeColor (int color, CPUGraph *base)
{
	GtkWidget *dialog;
	GtkColorSelection *colorsel;
	gint response;

	dialog = gtk_color_selection_dialog_new ("Select color");
	gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (base->m_OptionsDialog));

	colorsel = GTK_COLOR_SELECTION (GTK_COLOR_SELECTION_DIALOG (dialog)->colorsel);

	if (color == 0)
	{
		gtk_color_selection_set_previous_color (colorsel, &base->m_ForeGround1);
		gtk_color_selection_set_current_color (colorsel, &base->m_ForeGround1);	
	}
	else if (color == 1)
	{
		gtk_color_selection_set_previous_color (colorsel, &base->m_ForeGround2);
		gtk_color_selection_set_current_color (colorsel, &base->m_ForeGround2);
	}
	else if (color == 2)
	{
		gtk_color_selection_set_previous_color (colorsel, &base->m_BackGround);
		gtk_color_selection_set_current_color (colorsel, &base->m_BackGround);
	}
	else if (color == 3)
	{
		gtk_color_selection_set_previous_color (colorsel, &base->m_FrameColor);
		gtk_color_selection_set_current_color (colorsel, &base->m_FrameColor);
	}
	else if (color == 4)
	{
		gtk_color_selection_set_previous_color (colorsel, &base->m_ForeGround3);
		gtk_color_selection_set_current_color (colorsel, &base->m_ForeGround3);
	}

	gtk_color_selection_set_has_palette (colorsel, TRUE);

	response = gtk_dialog_run (GTK_DIALOG (dialog));
	if (response == GTK_RESPONSE_OK)
	{
		if (color == 0)
		{
			gtk_color_selection_get_current_color (colorsel, &base->m_ForeGround1);
			gtk_widget_modify_bg (base->m_Options.m_ColorDA, GTK_STATE_NORMAL, &base->m_ForeGround1);
		}
		else if (color == 1)
		{
			gtk_color_selection_get_current_color (colorsel, &base->m_ForeGround2);
			gtk_widget_modify_bg (base->m_Options.m_ColorDA2, GTK_STATE_NORMAL, &base->m_ForeGround2);
		}
		else if (color == 2)
		{
			gtk_color_selection_get_current_color (colorsel, &base->m_BackGround);
			gtk_widget_modify_bg (base->m_Options.m_ColorDA3, GTK_STATE_NORMAL, &base->m_BackGround);
		}
		else if (color == 3)
		{
			gtk_color_selection_get_current_color (colorsel, &base->m_FrameColor);
			gtk_widget_modify_bg (base->m_Options.m_ColorDA4, GTK_STATE_NORMAL, &base->m_FrameColor);
		}
		else if (color == 4)
		{
			gtk_color_selection_get_current_color (colorsel, &base->m_ForeGround3);
			gtk_widget_modify_bg (base->m_Options.m_ColorDA5, GTK_STATE_NORMAL, &base->m_ForeGround3);
		}
	}

	gtk_widget_destroy (dialog);

}

void SetHistorySize (CPUGraph *base, int size)
{
	base->m_History = (long *)realloc (base->m_History, size*sizeof (long));
	int i;
	for (i=size-1;i>=base->m_Values;i--)
		base->m_History[i] = 0;
	base->m_Values = size;

}

void ModeChange (GtkOptionMenu *om, CPUGraph *base)
{
	base->m_Mode = gtk_option_menu_get_history (om);
	if (base->m_Mode == 0)
	{
		if (base->m_ColorMode > 0)
			gtk_widget_set_sensitive (GTK_WIDGET (base->m_Options.m_FG2), TRUE);
		else
			gtk_widget_set_sensitive (GTK_WIDGET (base->m_Options.m_FG2), FALSE);
		gtk_widget_set_sensitive (GTK_WIDGET (base->m_Options.m_FG3), FALSE);
	}
	else if (base->m_Mode == 1)
	{
		if (base->m_ColorMode > 0)
			gtk_widget_set_sensitive (GTK_WIDGET (base->m_Options.m_FG3), TRUE);
		else
			gtk_widget_set_sensitive (GTK_WIDGET (base->m_Options.m_FG3), FALSE);
		gtk_widget_set_sensitive (GTK_WIDGET (base->m_Options.m_FG2), TRUE);
	}
	else if (base->m_Mode == 2)
	{
	        if (base->m_ColorMode > 0)
	                gtk_widget_set_sensitive (GTK_WIDGET (base->m_Options.m_FG2), TRUE);
	        else
	                gtk_widget_set_sensitive (GTK_WIDGET (base->m_Options.m_FG2), FALSE);							
		gtk_widget_set_sensitive (GTK_WIDGET (base->m_Options.m_FG3), FALSE);
	}
}
void UpdateChange (GtkOptionMenu *om, CPUGraph *base)
{
	base->m_UpdateInterval = gtk_option_menu_get_history (om);
}
void FrameChange (GtkToggleButton *button, CPUGraph *base)
{
	base->m_Frame = gtk_toggle_button_get_active (button);
	gtk_widget_set_sensitive (GTK_WIDGET (base->m_Options.m_FC), base->m_Frame);
	UserSetSize (base);
}

void ColorModeChange (GtkOptionMenu *om, CPUGraph *base)
{
	base->m_ColorMode = gtk_option_menu_get_history (om);
        
   	if (base->m_ColorMode == 0)
	{
		if (base->m_Mode == 0 || base->m_Mode == 2)
	      		 gtk_widget_set_sensitive (GTK_WIDGET (base->m_Options.m_FG2), FALSE);
		else
			gtk_widget_set_sensitive (GTK_WIDGET (base->m_Options.m_FG2), TRUE);
		gtk_widget_set_sensitive (GTK_WIDGET (base->m_Options.m_FG3), FALSE);
	}
	else if (base->m_ColorMode == 1)
	{
		gtk_widget_set_sensitive (GTK_WIDGET (base->m_Options.m_FG2), TRUE);
		if (base->m_Mode == 1)
			gtk_widget_set_sensitive (GTK_WIDGET (base->m_Options.m_FG3), TRUE);
		else
			gtk_widget_set_sensitive (GTK_WIDGET (base->m_Options.m_FG3), FALSE);
   	}
	else if (base->m_ColorMode == 2)
	{
		gtk_widget_set_sensitive (GTK_WIDGET (base->m_Options.m_FG2), TRUE);
	        if (base->m_Mode == 1)
        	        gtk_widget_set_sensitive (GTK_WIDGET (base->m_Options.m_FG3), TRUE);
	        else
        	        gtk_widget_set_sensitive (GTK_WIDGET (base->m_Options.m_FG3), FALSE);						
	}
}

XFCE_PLUGIN_CHECK_INIT
