/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Cirilo Bernardo
 * Copyright (C) 2016-2023 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <wx/log.h>
#include <wx/stdpaths.h>
#include <wx/process.h>
#include <wx/string.h>
#include <wx/filedlg.h>

#include <pgm_base.h>
#include <board.h>
#include <confirm.h>
#include <bitmaps.h>
#include <widgets/std_bitmap_button.h>
#include "dialog_export_step_base.h"
#include "dialog_export_step_process.h"
#include <footprint.h>
#include <kiface_base.h>
#include <locale_io.h>
#include <math/vector3.h>
#include <pcb_edit_frame.h>
#include <pcbnew_settings.h>
#include <project/project_file.h> // LAST_PATH_TYPE
#include <reporter.h>
#include <trace_helpers.h>
#include <widgets/text_ctrl_eval.h>
#include <wildcards_and_files_ext.h>
#include <filename_resolver.h>
#include <settings/settings_manager.h>


class DIALOG_EXPORT_STEP : public DIALOG_EXPORT_STEP_BASE
{
public:
    enum STEP_ORIGIN_OPTION
    {
        STEP_ORIGIN_0,             // absolute coordinates
        STEP_ORIGIN_PLOT_AXIS,     // origin is plot/drill axis origin
        STEP_ORIGIN_GRID_AXIS,     // origin is grid origin
        STEP_ORIGIN_BOARD_CENTER,  // origin is board center
        STEP_ORIGIN_USER,          // origin is entered by user
    };

    DIALOG_EXPORT_STEP( PCB_EDIT_FRAME* aParent, const wxString& aBoardPath );
    ~DIALOG_EXPORT_STEP();

protected:
    void onBrowseClicked( wxCommandEvent& aEvent ) override;
    void onUpdateUnits( wxUpdateUIEvent& aEvent ) override;
    void onUpdateXPos( wxUpdateUIEvent& aEvent ) override;
    void onUpdateYPos( wxUpdateUIEvent& aEvent ) override;
    void onExportButton( wxCommandEvent& aEvent ) override;

    int GetOrgUnitsChoice() const
    {
        return m_STEP_OrgUnitChoice->GetSelection();
    }

    double GetXOrg() const
    {
        return EDA_UNIT_UTILS::UI::DoubleValueFromString( m_STEP_Xorg->GetValue() );
    }

    double GetYOrg()
    {
        return EDA_UNIT_UTILS::UI::DoubleValueFromString( m_STEP_Yorg->GetValue() );
    }

    STEP_ORIGIN_OPTION GetOriginOption();

    bool GetNoUnspecifiedOption()
    {
        return m_cbRemoveUnspecified->GetValue();
    }

    bool GetNoDNPOption()
    {
        return m_cbRemoveDNP->GetValue();
    }

    bool GetSubstOption()
    {
        return m_cbSubstModels->GetValue();
    }

    bool GetOverwriteFile()
    {
        return m_cbOverwriteFile->GetValue();
    }

private:
    PCB_EDIT_FRAME*    m_parent;
    STEP_ORIGIN_OPTION m_origin;         // The last preference for STEP origin option
    double             m_userOriginX;    // remember last User Origin X value
    double             m_userOriginY;    // remember last User Origin Y value
    int                m_originUnits;    // remember last units for User Origin
    bool               m_noUnspecified;  // remember last preference for No Unspecified Component
    bool               m_noDNP;          // remember last preference for No DNP Component
    static bool        m_optimizeStep;   // remember last preference for Optimize STEP file (stored only for the session)
    static bool        m_exportTracks;   // remember last preference to export tracks (stored only for the session)
    static bool        m_exportZones;    // remember last preference to export tracks (stored only for the session)
    wxString           m_boardPath;      // path to the exported board file
    static int         m_toleranceLastChoice;  // Store m_tolerance option during a session
};


int  DIALOG_EXPORT_STEP::m_toleranceLastChoice = -1;     // Use default
bool DIALOG_EXPORT_STEP::m_optimizeStep = true;
bool DIALOG_EXPORT_STEP::m_exportTracks = false;
bool DIALOG_EXPORT_STEP::m_exportZones = false;

DIALOG_EXPORT_STEP::DIALOG_EXPORT_STEP( PCB_EDIT_FRAME* aParent, const wxString& aBoardPath ) :
    DIALOG_EXPORT_STEP_BASE( aParent )
{
    m_browseButton->SetBitmap( KiBitmapBundle( BITMAPS::small_folder ) );

    m_parent = aParent;
    m_boardPath = aBoardPath;

    SetupStandardButtons( { { wxID_OK,     _( "Export" ) },
                            { wxID_CANCEL, _( "Close" )  } } );

    // Build default output file name
    // (last saved filename in project or built from board filename)
    wxString path = m_parent->GetLastPath( LAST_PATH_STEP );

    if( path.IsEmpty() )
    {
        wxFileName brdFile( m_parent->GetBoard()->GetFileName() );
        brdFile.SetExt( wxT( "step" ) );
        path = brdFile.GetFullPath();
    }

    m_outputFileName->SetValue( path );

    Layout();
    bSizerSTEPFile->Fit( this );

    SetFocus();

    PCBNEW_SETTINGS* cfg = m_parent->GetPcbNewSettings();

    m_origin = static_cast<STEP_ORIGIN_OPTION>( cfg->m_ExportStep.origin_mode );

    switch( m_origin )
    {
    default:
    case STEP_ORIGIN_PLOT_AXIS:    m_rbDrillAndPlotOrigin->SetValue( true ); break;
    case STEP_ORIGIN_GRID_AXIS:    m_rbGridOrigin->SetValue( true );         break;
    case STEP_ORIGIN_USER:         m_rbUserDefinedOrigin->SetValue( true );  break;
    case STEP_ORIGIN_BOARD_CENTER: m_rbBoardCenterOrigin->SetValue( true );  break;
    }

    m_originUnits = cfg->m_ExportStep.origin_units;
    m_userOriginX = cfg->m_ExportStep.origin_x;
    m_userOriginY = cfg->m_ExportStep.origin_y;
    m_noUnspecified = cfg->m_ExportStep.no_unspecified;
    m_noDNP       = cfg->m_ExportStep.no_dnp;

    m_cbOptimizeStep->SetValue( m_optimizeStep );
    m_cbExportTracks->SetValue( m_exportTracks );
    m_cbExportZones->SetValue( m_exportZones );
    m_cbRemoveUnspecified->SetValue( m_noUnspecified );
    m_cbRemoveDNP->SetValue( m_noDNP );
    m_cbSubstModels->SetValue( cfg->m_ExportStep.replace_models );
    m_cbOverwriteFile->SetValue( cfg->m_ExportStep.overwrite_file );

    m_STEP_OrgUnitChoice->SetSelection( m_originUnits );
    wxString tmpStr;
    tmpStr << m_userOriginX;
    m_STEP_Xorg->SetValue( tmpStr );
    tmpStr = wxEmptyString;
    tmpStr << m_userOriginY;
    m_STEP_Yorg->SetValue( tmpStr );

    wxString bad_scales;
    size_t   bad_count = 0;

    for( FOOTPRINT* fp : aParent->GetBoard()->Footprints() )
    {
        for( const FP_3DMODEL& model : fp->Models() )
        {
            if( model.m_Scale.x != 1.0 || model.m_Scale.y != 1.0 || model.m_Scale.z != 1.0 )
            {
                bad_scales.Append( wxS("\n") );
                bad_scales.Append( model.m_Filename );
                bad_count++;
            }
        }

        if( bad_count >= 5 )
            break;
    }

    if( !bad_scales.empty()
            && !Pgm().GetCommonSettings()->m_DoNotShowAgain.scaled_3d_models_warning )
    {
        wxString extendedMsg = _( "Non-unity scaled models:" ) + wxT( "\n" ) + bad_scales;

        KIDIALOG msgDlg( m_parent, _( "Scaled models detected.  "
                                      "Model scaling is not reliable for mechanical export." ),
                         _( "Model Scale Warning" ), wxOK | wxICON_WARNING );
        msgDlg.SetExtendedMessage( extendedMsg );
        msgDlg.DoNotShowCheckbox( __FILE__, __LINE__ );

        msgDlg.ShowModal();

        if( msgDlg.DoNotShowAgain() )
            Pgm().GetCommonSettings()->m_DoNotShowAgain.scaled_3d_models_warning = true;
    }

    if( m_toleranceLastChoice >= 0 )
        m_choiceTolerance->SetSelection( m_toleranceLastChoice );

    // Now all widgets have the size fixed, call FinishDialogSettings
    finishDialogSettings();
}


DIALOG_EXPORT_STEP::~DIALOG_EXPORT_STEP()
{
    GetOriginOption(); // Update m_origin member.

    PCBNEW_SETTINGS* cfg = nullptr;

    try
    {
        cfg = m_parent->GetPcbNewSettings();
    }
    catch( const std::runtime_error& e )
    {
        wxFAIL_MSG( e.what() );
    }

    if( cfg )
    {
        cfg->m_ExportStep.origin_mode = static_cast<int>( m_origin );
        cfg->m_ExportStep.origin_units = m_STEP_OrgUnitChoice->GetSelection();
        cfg->m_ExportStep.replace_models = m_cbSubstModels->GetValue();
        cfg->m_ExportStep.overwrite_file = m_cbOverwriteFile->GetValue();

        double val = 0.0;

        m_STEP_Xorg->GetValue().ToDouble( &val );
        cfg->m_ExportStep.origin_x = val;

        m_STEP_Yorg->GetValue().ToDouble( &val );
        cfg->m_ExportStep.origin_y = val;

        cfg->m_ExportStep.no_unspecified = m_cbRemoveUnspecified->GetValue();
        cfg->m_ExportStep.no_dnp = m_cbRemoveDNP->GetValue();
    }

    m_toleranceLastChoice = m_choiceTolerance->GetSelection();
    m_optimizeStep = m_cbOptimizeStep->GetValue();
    m_exportTracks = m_cbExportTracks->GetValue();
    m_exportZones = m_cbExportZones->GetValue();
}


DIALOG_EXPORT_STEP::STEP_ORIGIN_OPTION DIALOG_EXPORT_STEP::GetOriginOption()
{
    m_origin = STEP_ORIGIN_0;

    if( m_rbDrillAndPlotOrigin->GetValue() )
        m_origin = STEP_ORIGIN_PLOT_AXIS;
    else if( m_rbGridOrigin->GetValue() )
        m_origin = STEP_ORIGIN_GRID_AXIS;
    else if( m_rbUserDefinedOrigin->GetValue() )
        m_origin = STEP_ORIGIN_USER;
    else if( m_rbBoardCenterOrigin->GetValue() )
        m_origin = STEP_ORIGIN_BOARD_CENTER;

    return m_origin;
}


void PCB_EDIT_FRAME::OnExportSTEP( wxCommandEvent& event )
{
    wxFileName brdFile = GetBoard()->GetFileName();

    // The project filename (.kicad_pro) of the auto saved board filename, if it is created
    wxFileName autosaveProjFile;

    if( GetScreen()->IsContentModified() || brdFile.GetFullPath().empty() )
    {
        if( !doAutoSave() )
        {
            DisplayErrorMessage( this, _( "STEP export failed!  "
                                          "Please save the PCB and try again" ) );
            return;
        }

        wxString autosaveFileName = GetAutoSaveFilePrefix() + brdFile.GetName();

        // Create a dummy .kicad_pro file for this auto saved board file.
        // this is useful to use some settings (like project path and name)
        // Because doAutoSave() works, the target directory exists and is writable
        autosaveProjFile = brdFile;
        autosaveProjFile.SetName( autosaveFileName );
        autosaveProjFile.SetExt( "kicad_pro" );

        // Use auto-saved board for export
        GetSettingsManager()->SaveProjectCopy( autosaveProjFile.GetFullPath(), GetBoard()->GetProject() );
        brdFile.SetName( autosaveFileName );
    }

    DIALOG_EXPORT_STEP dlg( this, brdFile.GetFullPath() );
    dlg.ShowModal();

    // If a dummy .kicad_pro file is created, delete it now it is useless.
    if( !autosaveProjFile.GetFullPath().IsEmpty() )
        wxRemoveFile( autosaveProjFile.GetFullPath() );
}


void DIALOG_EXPORT_STEP::onUpdateUnits( wxUpdateUIEvent& aEvent )
{
    aEvent.Enable( m_rbUserDefinedOrigin->GetValue() );
}


void DIALOG_EXPORT_STEP::onUpdateXPos( wxUpdateUIEvent& aEvent )
{
    aEvent.Enable( m_rbUserDefinedOrigin->GetValue() );
}


void DIALOG_EXPORT_STEP::onUpdateYPos( wxUpdateUIEvent& aEvent )
{
    aEvent.Enable( m_rbUserDefinedOrigin->GetValue() );
}


void DIALOG_EXPORT_STEP::onBrowseClicked( wxCommandEvent& aEvent )
{
    wxString filter = _( "STEP files" )
                      + AddFileExtListToFilter( { FILEEXT::StepFileExtension, FILEEXT::StepFileAbrvExtension } ) + "|"
                      + _( "Binary glTF files" )
                      + AddFileExtListToFilter( { FILEEXT::GltfBinaryFileExtension } );

    // Build the absolute path of current output directory to preselect it in the file browser.
    wxString   path = ExpandEnvVarSubstitutions( m_outputFileName->GetValue(), &Prj() );
    wxFileName fn( Prj().AbsolutePath( path ) );

    wxFileDialog dlg( this, _( "STEP Output File" ), fn.GetPath(), fn.GetFullName(), filter,
                      wxFD_SAVE );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    m_outputFileName->SetValue( dlg.GetPath() );
}


void DIALOG_EXPORT_STEP::onExportButton( wxCommandEvent& aEvent )
{
    wxString path = m_outputFileName->GetValue();
    m_parent->SetLastPath( LAST_PATH_STEP, path );

    // Build the absolute path of current output directory to preselect it in the file browser.
    std::function<bool( wxString* )> textResolver =
            [&]( wxString* token ) -> bool
            {
                return m_parent->GetBoard()->ResolveTextVar( token, 0 );
            };

    path = ExpandTextVars( path, &textResolver );
    path = ExpandEnvVarSubstitutions( path, &Prj() );
    path = Prj().AbsolutePath( path );

    if( path.IsEmpty() )
    {
        DisplayErrorMessage( this, _( "No filename for output file" ) );
        return;
    }

    double tolerance;   // default value in mm
    m_toleranceLastChoice = m_choiceTolerance->GetSelection();
    m_optimizeStep = m_cbOptimizeStep->GetValue();
    m_exportTracks = m_cbExportTracks->GetValue();
    m_exportZones = m_cbExportZones->GetValue();

    switch( m_choiceTolerance->GetSelection() )
    {
    case 0:  tolerance = 0.001; break;
    default:
    case 1:  tolerance = 0.01;  break;
    case 2:  tolerance = 0.1;   break;
    }

    SHAPE_POLY_SET outline;
    wxString msg;

    // Check if the board outline is continuous
    // max dist from one endPt to next startPt to build a closed shape:
    int chainingEpsilon = pcbIUScale.mmToIU( tolerance );

    // Arc to segment approx error (not critical here: we do not use the outline shape):
    int maxError = pcbIUScale.mmToIU( 0.005 );
    bool success = BuildBoardPolygonOutlines( m_parent->GetBoard(), outline, maxError,
                                              chainingEpsilon, nullptr );
    if( !success )
    {
        DisplayErrorMessage( this, wxString::Format(
                             _( "Board outline is missing or not closed using %.3f mm tolerance.\n"
                                "Run DRC for a full analysis." ), tolerance ) );
        return;
    }

    wxFileName fn( Prj().AbsolutePath( path ) );

    if( fn.FileExists() && !GetOverwriteFile() )
    {
        msg.Printf( _( "File '%s' already exists. Do you want overwrite this file?" ),
                    fn.GetFullPath() );

        if( wxMessageBox( msg, _( "STEP/GLTF Export" ), wxYES_NO | wxICON_QUESTION, this ) == wxNO )
            return;
    }

    wxFileName appK2S( wxStandardPaths::Get().GetExecutablePath() );
#ifdef __WXMAC__
    // On macOS, we have standalone applications inside the main bundle, so we handle that here:
    if( appK2S.GetPath().Find( "/Contents/Applications/pcbnew.app/Contents/MacOS" ) != wxNOT_FOUND )
    {
        appK2S.AppendDir( wxT( ".." ) );
        appK2S.AppendDir( wxT( ".." ) );
        appK2S.AppendDir( wxT( ".." ) );
        appK2S.AppendDir( wxT( ".." ) );
        appK2S.AppendDir( wxT( "MacOS" ) );
    }
#else
    if( wxGetEnv( wxT( "KICAD_RUN_FROM_BUILD_DIR" ), nullptr ) )
    {
        appK2S.RemoveLastDir();
        appK2S.AppendDir( "kicad" );
    }
#endif

    appK2S.SetName( wxT( "kicad-cli" ) );
    appK2S.Normalize( FN_NORMALIZE_FLAGS );

    wxString cmdK2S = wxT( "\"" );
    cmdK2S.Append( appK2S.GetFullPath() );
    cmdK2S.Append( wxT( "\"" ) );

    cmdK2S.Append( wxT( " pcb" ) );
    cmdK2S.Append( wxT( " export" ) );

    if( fn.GetExt() == FILEEXT::GltfBinaryFileExtension )
        cmdK2S.Append( wxT( " glb" ) );
    else
        cmdK2S.Append( wxT( " step" ) );

    if( GetNoUnspecifiedOption() )
        cmdK2S.Append( wxT( " --no-unspecified" ) );

    if( GetNoDNPOption() )
        cmdK2S.Append( wxT( " --no-dnp" ) );

    if( GetSubstOption() )
        cmdK2S.Append( wxT( " --subst-models" ) );

    if( !m_optimizeStep )
        cmdK2S.Append( wxT( " --no-optimize-step" ) );

    if( m_exportTracks )
        cmdK2S.Append( wxT( " --include-tracks" ) );

    if( m_exportZones )
        cmdK2S.Append( wxT( " --include-zones" ) );

    // Note: for some reason, using \" to insert a quote in a format string, under MacOS
    // wxString::Format does not work. So use a %c format in string
    int quote = '\'';
    int dblquote = '"';

    switch( GetOriginOption() )
    {
    case STEP_ORIGIN_0:
        wxFAIL_MSG( wxT( "Unsupported origin option: how did we get here?" ) );
        break;

    case STEP_ORIGIN_PLOT_AXIS:
        cmdK2S.Append( wxT( " --drill-origin" ) );
        break;

    case STEP_ORIGIN_GRID_AXIS:
        cmdK2S.Append( wxT( " --grid-origin" ) );
        break;

    case STEP_ORIGIN_USER:
    {
        double xOrg = GetXOrg();
        double yOrg = GetYOrg();

        if( GetOrgUnitsChoice() == 1 )
        {
            // selected reference unit is in inches, and STEP units are mm
            xOrg *= 25.4;
            yOrg *= 25.4;
        }

        LOCALE_IO dummy;
        cmdK2S.Append( wxString::Format( wxT( " --user-origin=%c%.6fx%.6fmm%c" ),
                                         quote, xOrg, yOrg, quote ) );
        break;
    }

    case STEP_ORIGIN_BOARD_CENTER:
    {
        BOX2I     bbox = m_parent->GetBoard()->ComputeBoundingBox( true );
        double    xOrg = pcbIUScale.IUTomm( bbox.GetCenter().x );
        double    yOrg = pcbIUScale.IUTomm( bbox.GetCenter().y );
        LOCALE_IO dummy;

        cmdK2S.Append( wxString::Format( wxT( " --user-origin=%c%.6fx%.6fmm%c" ),
                                         quote, xOrg, yOrg, quote ) );
        break;
    }
    }

    {
        LOCALE_IO dummy;
        cmdK2S.Append( wxString::Format( wxT( " --min-distance=%c%.3fmm%c" ),
                                         quote, tolerance, quote ) );
    }

    // Output file path.
    cmdK2S.Append( wxString::Format( wxT( " -f -o %c%s%c" ),
                                     dblquote, fn.GetFullPath(), dblquote ) );


    // Input file path.
    cmdK2S.Append( wxString::Format( wxT( " %c%s%c" ), dblquote, m_boardPath, dblquote ) );

    wxLogTrace( traceKiCad2Step, wxT( "export step command: %s" ), cmdK2S );

    DIALOG_EXPORT_STEP_LOG* log = new DIALOG_EXPORT_STEP_LOG( this, cmdK2S );
    log->ShowModal();
    Close();
}
