#include <wx/wx.h>

#include <system/config.h>
#include <system/loader.h>

#include "launcher.h"


CChoiceList::CChoiceList()
{
	m_list = new wxList();
	m_list->DeleteContents(true);
}

CChoiceList::~CChoiceList()
{
	m_list->Clear();
	delete m_list;
}

void CChoiceList::SetControls(wxChoice* choice, wxTextCtrl* textctrl)
{
	m_choice = choice;
	m_textctrl = textctrl;
}

void CChoiceList::AddChoice(const wxString& describtion, const wxString& value)
{
	CChoiceElement* element = new CChoiceElement;

	element->value = value;
	element->describtion = describtion;

	m_list->Append(element);
}

void CChoiceList::Update()
{
	if (m_textctrl != 0)
	if (m_choice->GetCurrentSelection() == 0) m_textctrl->Enable(true);
	else m_textctrl->Enable(false);
}

wxArrayString CChoiceList::GetChoices()
{
	wxArrayString array_string;

	size_t count = m_list->GetCount();

	array_string.Alloc(count);

	for (size_t i = 0; i < count; i++)
	{
		CChoiceElement* element = dynamic_cast<CChoiceElement*>(m_list->Item(i)->GetData());

		wxString format;
		wxString describtion = element->describtion;
		wxString value = element->value;

		if (value == _T("")) format = _T("%s");
		else format = _T("%s (%s)");

		array_string.Add(wxString::Format(format, describtion, value));
	}

	return array_string;
}

wxString CChoiceList::GetCurrentChoice(const wxString& format)
{
	wxString retval;

	size_t index = m_choice->GetCurrentSelection();

	if (index != 0 || m_textctrl == 0) retval = dynamic_cast<CChoiceElement*>(m_list->Item(index)->GetData())->value;
	else retval = m_textctrl->GetValue();

	if (retval != _T("")) retval = wxString::Format(format, retval);
	else retval = _T("");

	return retval;
}


CLauncherDialog::CLauncherDialog(wxWindow* parent) : wxDialog( parent, wxID_ANY,
	_T("Half-Life Advanced Effects - Launcher"), wxDefaultPosition, wxSize(410,402))
{
	// Initialize

	m_depthchoices = new CChoiceList();
	m_modchoices = new CChoiceList();
	m_capturemodechoices = new CChoiceList();


	// Prepare

	m_modchoices->AddChoice(_T("(Custom)"), _T(""));
	m_modchoices->AddChoice(_T("Half-Life"), _T(""));
	m_modchoices->AddChoice(_T("Counter-Strike"), _T("cstrike"));
	m_modchoices->AddChoice(_T("Day of Defeat"), _T("dod"));
	m_modchoices->AddChoice(_T("Team Fortress Classic"), _T("tcf"));

	m_depthchoices->AddChoice(_T("(Custom)"), _T(""));
	m_depthchoices->AddChoice(_T("High"), _T("32"));
	m_depthchoices->AddChoice(_T("Medium"), _T("24"));
	m_depthchoices->AddChoice(_T("Low"), _T("16"));

	m_capturemodechoices->AddChoice(_T("None"), _T(""));
	m_capturemodechoices->AddChoice(_T("Undock"), _T(""));
	m_capturemodechoices->AddChoice(_T("Framebufferobject"), _T("fbo"));
	m_capturemodechoices->AddChoice(_T("MemoryDC"), _T("memdc"));


	// Create the form (code generated by wxFormBuilder)

	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bs_main;
	bs_main = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bs_presets;
	bs_presets = new wxBoxSizer( wxHORIZONTAL );
	
	wxStaticText* st_presets;
	st_presets = new wxStaticText( this, wxID_ANY, wxT("Presets:"), wxDefaultPosition, wxDefaultSize, 0 );
	st_presets->Wrap( -1 );
	st_presets->Enable( false );
	
	bs_presets->Add( st_presets, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_cb_preset = new wxComboBox( this, wxID_ANY, wxT("Default"), wxDefaultPosition, wxDefaultSize, 0, NULL, 0 ); 
	m_cb_preset->Enable( false );
	
	bs_presets->Add( m_cb_preset, 1, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );
	
	wxButton* bt_save;
	bt_save = new wxButton( this, ID_SavePreset, wxT("Save"), wxDefaultPosition, wxDefaultSize, 0 );
	bt_save->Enable( false );
	
	bs_presets->Add( bt_save, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );
	
	wxButton* bt_delete;
	bt_delete = new wxButton( this, ID_DeletePreset, wxT("Delete"), wxDefaultPosition, wxDefaultSize, 0 );
	bt_delete->Enable( false );
	
	bs_presets->Add( bt_delete, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );
	
	bs_main->Add( bs_presets, 0, wxALL|wxEXPAND, 5 );
	
	wxStaticLine* sl_presets;
	sl_presets = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bs_main->Add( sl_presets, 0, wxEXPAND, 5 );
	
	wxStaticBoxSizer* sbs_game;
	sbs_game = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, wxT("Game") ), wxVERTICAL );
	
	wxFlexGridSizer* fgs_game;
	fgs_game = new wxFlexGridSizer( 4, 2, 5, 5 );
	fgs_game->AddGrowableCol( 1 );
	fgs_game->AddGrowableRow( 4 );
	fgs_game->SetFlexibleDirection( wxBOTH );
	fgs_game->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	wxStaticText* st_path;
	st_path = new wxStaticText( this, wxID_ANY, wxT("Half-Life Directory:"), wxDefaultPosition, wxDefaultSize, 0 );
	st_path->Wrap( -1 );
	fgs_game->Add( st_path, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	wxBoxSizer* bs_path;
	bs_path = new wxBoxSizer( wxHORIZONTAL );
	
	m_tc_path = new wxTextCtrl( this, ID_Changed, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bs_path->Add( m_tc_path, 1, wxALIGN_CENTER_VERTICAL, 5 );
	
	wxButton* bt_browse;
	bt_browse = new wxButton( this, ID_Browse, wxT("..."), wxDefaultPosition, wxSize( -1,-1 ), wxBU_EXACTFIT );
	bs_path->Add( bt_browse, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );
	
	fgs_game->Add( bs_path, 0, wxEXPAND, 5 );
	
	wxStaticText* st_mod;
	st_mod = new wxStaticText( this, wxID_ANY, wxT("Modification:"), wxDefaultPosition, wxDefaultSize, 0 );
	st_mod->Wrap( -1 );
	fgs_game->Add( st_mod, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	wxBoxSizer* bs_mod;
	bs_mod = new wxBoxSizer( wxHORIZONTAL );
	
	m_c_mod = new wxChoice( this, ID_ChoiceMod, wxDefaultPosition, wxDefaultSize, m_modchoices->GetChoices(), 0 );
	m_c_mod->SetSelection( 0 );
	bs_mod->Add( m_c_mod, 2, wxALIGN_CENTER_VERTICAL, 5 );
	
	wxStaticText* st_custommod;
	st_custommod = new wxStaticText( this, wxID_ANY, wxT("Custom:"), wxDefaultPosition, wxDefaultSize, 0 );
	st_custommod->Wrap( -1 );
	bs_mod->Add( st_custommod, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 10 );
	
	m_tc_mod = new wxTextCtrl( this, ID_Changed, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bs_mod->Add( m_tc_mod, 1, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );
	
	fgs_game->Add( bs_mod, 1, wxEXPAND, 5 );
	
	sbs_game->Add( fgs_game, 1, wxALL|wxEXPAND, 5 );
	
	bs_main->Add( sbs_game, 0, wxALL|wxEXPAND, 5 );
	
	wxStaticBoxSizer* sbs_res;
	sbs_res = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, wxT("Resolution") ), wxHORIZONTAL );
	
	wxBoxSizer* bs_res;
	bs_res = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bSizer11;
	bSizer11 = new wxBoxSizer( wxVERTICAL );
	
	m_ch_force = new wxCheckBox( this, ID_Changed, wxT("Force"), wxDefaultPosition, wxDefaultSize, 0 );
	
	bSizer11->Add( m_ch_force, 0, 0, 5 );
	
	bs_res->Add( bSizer11, 3, wxALIGN_CENTER_VERTICAL, 5 );
	
	wxBoxSizer* bs_width;
	bs_width = new wxBoxSizer( wxVERTICAL );
	
	wxStaticText* st_width;
	st_width = new wxStaticText( this, wxID_ANY, wxT("Width:"), wxDefaultPosition, wxDefaultSize, 0 );
	st_width->Wrap( -1 );
	bs_width->Add( st_width, 0, 0, 5 );
	
	m_tc_width = new wxTextCtrl( this, ID_Changed, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bs_width->Add( m_tc_width, 0, wxEXPAND|wxTOP, 5 );
	
	bs_res->Add( bs_width, 3, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxLEFT, 5 );
	
	wxBoxSizer* bs_height;
	bs_height = new wxBoxSizer( wxVERTICAL );
	
	wxStaticText* st_height;
	st_height = new wxStaticText( this, wxID_ANY, wxT("Height:"), wxDefaultPosition, wxDefaultSize, 0 );
	st_height->Wrap( -1 );
	bs_height->Add( st_height, 0, 0, 5 );
	
	m_tc_height = new wxTextCtrl( this, ID_Changed, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bs_height->Add( m_tc_height, 0, wxEXPAND|wxTOP, 5 );
	
	bs_res->Add( bs_height, 3, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxLEFT, 5 );
	
	wxBoxSizer* bs_depth;
	bs_depth = new wxBoxSizer( wxVERTICAL );
	
	wxStaticText* st_depth;
	st_depth = new wxStaticText( this, wxID_ANY, wxT("Color Depth:"), wxDefaultPosition, wxDefaultSize, 0 );
	st_depth->Wrap( -1 );
	bs_depth->Add( st_depth, 0, 0, 10 );
	
	m_c_depth = new wxChoice( this, ID_ChoiceDepth, wxDefaultPosition, wxDefaultSize, m_depthchoices->GetChoices(), 0 );
	m_c_depth->SetSelection( 0 );
	bs_depth->Add( m_c_depth, 0, wxEXPAND|wxTOP, 5 );
	
	bs_res->Add( bs_depth, 5, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxLEFT, 10 );
	
	wxBoxSizer* bs_customdepth;
	bs_customdepth = new wxBoxSizer( wxVERTICAL );
	
	wxStaticText* st_depthcustom;
	st_depthcustom = new wxStaticText( this, wxID_ANY, wxT("Custom:"), wxDefaultPosition, wxDefaultSize, 0 );
	st_depthcustom->Wrap( -1 );
	bs_customdepth->Add( st_depthcustom, 0, 0, 5 );
	
	m_tc_depth = new wxTextCtrl( this, ID_Changed, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bs_customdepth->Add( m_tc_depth, 0, wxEXPAND|wxTOP, 5 );
	
	bs_res->Add( bs_customdepth, 3, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxLEFT, 5 );
	
	wxBoxSizer* bs_customdepth1;
	bs_customdepth1 = new wxBoxSizer( wxVERTICAL );
	
	wxStaticText* st_capturemode;
	st_capturemode = new wxStaticText( this, wxID_ANY, wxT("Capture Mode:"), wxDefaultPosition, wxDefaultSize, 0 );
	st_capturemode->Wrap( -1 );
	bs_customdepth1->Add( st_capturemode, 0, 0, 5 );
	
	m_c_capturemode = new wxChoice( this, ID_ChoiceDepth, wxDefaultPosition, wxDefaultSize, m_capturemodechoices->GetChoices(), 0 );
	m_c_capturemode->SetSelection( 0 );
	bs_customdepth1->Add( m_c_capturemode, 0, wxEXPAND|wxTOP, 5 );
	
	bs_res->Add( bs_customdepth1, 5, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxLEFT, 10 );
	
	sbs_res->Add( bs_res, 1, wxALL|wxEXPAND, 5 );
	
	bs_main->Add( sbs_res, 0, wxALL|wxEXPAND, 5 );
	
	wxStaticBoxSizer* sbs_cmdline;
	sbs_cmdline = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, wxT("Commandline") ), wxVERTICAL );
	
	wxFlexGridSizer* fgs_cmdline;
	fgs_cmdline = new wxFlexGridSizer( 4, 2, 5, 5 );
	fgs_cmdline->AddGrowableCol( 1 );
	fgs_cmdline->SetFlexibleDirection( wxBOTH );
	fgs_cmdline->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_ALL );
	
	wxStaticText* st_additionalcmdline;
	st_additionalcmdline = new wxStaticText( this, wxID_ANY, wxT("Additional:"), wxDefaultPosition, wxDefaultSize, 0 );
	st_additionalcmdline->Wrap( -1 );
	fgs_cmdline->Add( st_additionalcmdline, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_tc_additionalcmdline = new wxTextCtrl( this, ID_Changed, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE );
	fgs_cmdline->Add( m_tc_additionalcmdline, 0, wxEXPAND, 5 );
	
	wxStaticText* st_fullcmdline;
	st_fullcmdline = new wxStaticText( this, wxID_ANY, wxT("Full:"), wxDefaultPosition, wxDefaultSize, 0 );
	st_fullcmdline->Wrap( -1 );
	fgs_cmdline->Add( st_fullcmdline, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_tc_fullcmdline = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE|wxTE_READONLY );
	fgs_cmdline->Add( m_tc_fullcmdline, 1, wxEXPAND, 5 );
	
	sbs_cmdline->Add( fgs_cmdline, 1, wxALL|wxEXPAND, 5 );
	
	bs_main->Add( sbs_cmdline, 0, wxALL|wxEXPAND, 5 );
	
	wxStaticLine* m_sl2;
	m_sl2 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bs_main->Add( m_sl2, 0, wxEXPAND, 5 );
	
	wxBoxSizer* bs_dglbuttons;
	bs_dglbuttons = new wxBoxSizer( wxHORIZONTAL );
	
	wxButton* bt_ok;
	bt_ok = new wxButton( this, ID_Launch, wxT("Launch"), wxDefaultPosition, wxDefaultSize, 0 );
	bt_ok->SetDefault(); 
	bs_dglbuttons->Add( bt_ok, 0, wxALIGN_RIGHT, 5 );
	
	wxButton* bt_cancel;
	bt_cancel = new wxButton( this, wxID_CANCEL, wxT("Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
	bs_dglbuttons->Add( bt_cancel, 0, wxALIGN_RIGHT|wxLEFT, 5 );
	
	bs_main->Add( bs_dglbuttons, 0, wxALIGN_RIGHT|wxALL, 5 );
	
	this->SetSizer( bs_main );
	this->Layout();


	// Create the events

	// Buttons
	Connect(ID_Browse, wxEVT_COMMAND_BUTTON_CLICKED,
		wxCommandEventHandler(CLauncherDialog::OnBrowse));
	Connect(ID_SavePreset, wxEVT_COMMAND_BUTTON_CLICKED,
		wxCommandEventHandler(CLauncherDialog::OnSavePreset));
	Connect(ID_DeletePreset, wxEVT_COMMAND_BUTTON_CLICKED,
		wxCommandEventHandler(CLauncherDialog::OnDeletePreset));
	Connect(ID_Launch, wxEVT_COMMAND_BUTTON_CLICKED,
		wxCommandEventHandler(CLauncherDialog::OnLaunch));
	// Other Events
	Connect(ID_Changed, wxEVT_COMMAND_TEXT_UPDATED,
		wxCommandEventHandler(CLauncherDialog::OnChanged));
	Connect(ID_Changed, wxEVT_COMMAND_CHECKBOX_CLICKED,
		wxCommandEventHandler(CLauncherDialog::OnChanged));
	Connect(ID_ChoiceMod, wxEVT_COMMAND_CHOICE_SELECTED,
		wxCommandEventHandler(CLauncherDialog::OnChanged));
	Connect(ID_ChoiceDepth, wxEVT_COMMAND_CHOICE_SELECTED,
		wxCommandEventHandler(CLauncherDialog::OnChanged));

	SetEscapeId(wxID_CANCEL);


	// Set controls

	m_modchoices->SetControls(m_c_mod, m_tc_mod);
	m_depthchoices->SetControls(m_c_depth, m_tc_depth);
	m_capturemodechoices->SetControls(m_c_capturemode, 0);


	// Load preset

	m_tc_path->SetValue(g_config.GetPropertyString(_T("launcher"), _T("path")));
	m_c_mod->SetSelection(g_config.GetPropertyInteger(_T("launcher"), _T("modsel")));
	m_c_depth->SetSelection(g_config.GetPropertyInteger(_T("launcher"), _T("depthsel")));
	m_tc_mod->SetValue(g_config.GetPropertyString(_T("launcher"), _T("mod")));
	m_tc_depth->SetValue(g_config.GetPropertyString(_T("launcher"), _T("depth")));
	m_tc_width->SetValue(g_config.GetPropertyString(_T("launcher"), _T("width")));
	m_tc_height->SetValue(g_config.GetPropertyString(_T("launcher"), _T("height")));
	m_tc_additionalcmdline->SetValue(g_config.GetPropertyString(_T("launcher"), _T("cmdline")));
	m_ch_force->SetValue(g_config.GetPropertyBoolean(_T("launcher"), _T("force")));
	m_c_capturemode->SetSelection(g_config.GetPropertyInteger(_T("launcher"), _T("capturemethod")));


	// Update

	m_modchoices->Update();
	m_depthchoices->Update();
	m_capturemodechoices->Update();

	UpdateCmdline();
}

CLauncherDialog::~CLauncherDialog()
{
	delete m_depthchoices;
	delete m_modchoices;
}

void CLauncherDialog::OnChanged(wxCommandEvent& evt)
{
	switch(evt.GetId())
	{
	case ID_ChoiceMod: 
		m_modchoices->Update();
		break;
	case ID_ChoiceDepth:
		m_depthchoices->Update();
		break;
	}
	
	UpdateCmdline();
}

void CLauncherDialog::OnBrowse(wxCommandEvent& WXUNUSED(evt))
{
	wxFileDialog* filedgl = new wxFileDialog(this, _T("Choose \"hl.exe\""),
		_T(HLAE_DEFAULTHLPATH),	_T("hl.exe"), _T("Half-Life executable (hl.exe)|hl.exe"),
		wxFD_OPEN | wxFD_FILE_MUST_EXIST);
	filedgl->ShowModal();
	wxString path = filedgl->GetPath();
	if (path != _T(HLAE_DEFAULTHLPATH))
	{
		m_path = path;
		m_tc_path->SetValue(path);
	}
}

void CLauncherDialog::UpdateCmdline()
{
	// Path
	
	m_path = m_tc_path->GetValue();


	// Standard parameters

	m_cmdline = _T("-steam -gl -window");


	// Mod parameter

	m_cmdline += m_modchoices->GetCurrentChoice(_T(" -game %s"));


	// Depth parameter

	m_depth = m_depthchoices->GetCurrentChoice(_T("%s"));

	if (m_depth != _T("")) m_cmdline += wxString::Format(_T(" -%sbpp"), m_depth);


	// Resolution parameter

	m_width = m_tc_width->GetValue();
	m_height = m_tc_height->GetValue();
	m_force = m_ch_force->GetValue();

	if (m_force)
	{
		if ((m_width != _T("")) && (m_height != _T("")) && (m_depth != _T("")))
			m_cmdline += wxString::Format(_T(" -mdtres %sx%sx%s"), m_width, m_height, m_depth);
	}
	else
	{
		if (m_width != _T("")) m_cmdline += wxString::Format(_T(" -w %s"), m_width);
		if (m_height != _T("")) m_cmdline += wxString::Format(_T(" -h %s"), m_height);
	}

	// Hlaerender parameter

	if (m_capturemodechoices->GetCurrentChoice(_T("%s")) != _T(""))
		m_cmdline += m_capturemodechoices->GetCurrentChoice(_T(" -hlaerender %s"));


	// Additional parameters

	m_additionalcmdline = wxString::Format(_T(" %s"), m_tc_additionalcmdline->GetValue());
	m_cmdline += m_additionalcmdline;


	// Write the full cmdline

	m_tc_fullcmdline->SetValue(m_cmdline);
}

void CLauncherDialog::OnLaunch(wxCommandEvent& WXUNUSED(evt))
{

	// Save preset

	g_config.SetPropertyString(_T("launcher"), _T("path"), m_tc_path->GetValue());
	g_config.SetPropertyInteger(_T("launcher"), _T("modsel"), m_c_mod->GetCurrentSelection());
	g_config.SetPropertyInteger(_T("launcher"), _T("depthsel"), m_c_depth->GetCurrentSelection());
	g_config.SetPropertyString(_T("launcher"), _T("mod"), m_tc_mod->GetValue());
	g_config.SetPropertyString(_T("launcher"), _T("depth"), m_tc_depth->GetValue());
	g_config.SetPropertyString(_T("launcher"), _T("width"), m_tc_width->GetValue());
	g_config.SetPropertyString(_T("launcher"), _T("height"), m_tc_height->GetValue());
	g_config.SetPropertyString(_T("launcher"), _T("cmdline"), m_tc_additionalcmdline->GetValue());
	g_config.SetPropertyBoolean(_T("launcher"), _T("force"), m_ch_force->GetValue());

	g_config.Flush();

	InitLoader(this, m_path, m_cmdline);

	Close();
}

void CLauncherDialog::OnSavePreset(wxCommandEvent& WXUNUSED(evt))
{
	// TODO
}

void CLauncherDialog::OnDeletePreset(wxCommandEvent& WXUNUSED(evt))
{
	// TODO
}
