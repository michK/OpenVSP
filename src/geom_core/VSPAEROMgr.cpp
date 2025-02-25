//
// This file is released under the terms of the NASA Open Source Agreement (NOSA)
// version 1.3 as detailed in the LICENSE file which accompanies this software.
//

// VSPAEROMgr.cpp: VSPAERO Mgr Singleton.
//
//////////////////////////////////////////////////////////////////////

#include "Defines.h"
#include "APIDefines.h"
#include "LinkMgr.h"
#include "MeshGeom.h"
#include "ParmMgr.h"
#include "StlHelper.h"
#include "Vehicle.h"
#include "VehicleMgr.h"
#include "VSPAEROMgr.h"
#include "WingGeom.h"

#include "StringUtil.h"
#include "FileUtil.h"

#include <regex>

//==== Constructor ====//
VspAeroControlSurf::VspAeroControlSurf()
{
    isGrouped = false;
}

//==== Constructor ====//
VSPAEROMgrSingleton::VSPAEROMgrSingleton() : ParmContainer()
{
    m_Name = "VSPAEROSettings";
    string groupname = "VSPAERO";

    m_GeomSet.Init( "GeomSet", groupname, this, 0, 0, 12 );
    m_GeomSet.SetDescript( "Geometry set" );

    m_AnalysisMethod.Init( "AnalysisMethod", groupname, this, vsp::VORTEX_LATTICE, vsp::VORTEX_LATTICE, vsp::PANEL );
    m_AnalysisMethod.SetDescript( "Analysis method: 0=VLM, 1=Panel" );

    m_LastPanelMeshGeomId = string();

    m_Sref.Init( "Sref", groupname, this, 100.0, 0.0, 1e12 );
    m_Sref.SetDescript( "Reference area" );

    m_bref.Init( "bref", groupname, this, 1.0, 0.0, 1e6 );
    m_bref.SetDescript( "Reference span" );

    m_cref.Init( "cref", groupname, this, 1.0, 0.0, 1e6 );
    m_cref.SetDescript( "Reference chord" );

    m_RefFlag.Init( "RefFlag", groupname, this, vsp::MANUAL_REF, 0, vsp::NUM_REF_TYPES - 1 );
    m_RefFlag.SetDescript( "Reference quantity flag" );

    m_CGGeomSet.Init( "MassSet", groupname, this, 0, 0, 12 );
    m_CGGeomSet.SetDescript( "Mass property set" );

    m_NumMassSlice.Init( "NumMassSlice", groupname, this, 10, 10, 200 );
    m_NumMassSlice.SetDescript( "Number of mass property slices" );

    m_Xcg.Init( "Xcg", groupname, this, 0.0, -1.0e12, 1.0e12 );
    m_Xcg.SetDescript( "X Center of Gravity" );

    m_Ycg.Init( "Ycg", groupname, this, 0.0, -1.0e12, 1.0e12 );
    m_Ycg.SetDescript( "Y Center of Gravity" );

    m_Zcg.Init( "Zcg", groupname, this, 0.0, -1.0e12, 1.0e12 );
    m_Zcg.SetDescript( "Z Center of Gravity" );


    // Flow Condition
    m_AlphaStart.Init( "AlphaStart", groupname, this, 1.0, -180, 180 );
    m_AlphaStart.SetDescript( "Angle of attack (Start)" );
    m_AlphaEnd.Init( "AlphaEnd", groupname, this, 10.0, -180, 180 );
    m_AlphaEnd.SetDescript( "Angle of attack (End)" );
    m_AlphaNpts.Init( "AlphaNpts", groupname, this, 3, 1, 100 );
    m_AlphaNpts.SetDescript( "Angle of attack (Num Points)" );

    m_BetaStart.Init( "BetaStart", groupname, this, 0.0, -180, 180 );
    m_BetaStart.SetDescript( "Angle of sideslip (Start)" );
    m_BetaEnd.Init( "BetaEnd", groupname, this, 0.0, -180, 180 );
    m_BetaEnd.SetDescript( "Angle of sideslip (End)" );
    m_BetaNpts.Init( "BetaNpts", groupname, this, 1, 1, 100 );
    m_BetaNpts.SetDescript( "Angle of sideslip (Num Points)" );

    m_MachStart.Init( "MachStart", groupname, this, 0.0, 0.0, 5.0 );
    m_MachStart.SetDescript( "Freestream Mach number (Start)" );
    m_MachEnd.Init( "MachEnd", groupname, this, 0.0, 0.0, 5.0 );
    m_MachEnd.SetDescript( "Freestream Mach number (End)" );
    m_MachNpts.Init( "MachNpts", groupname, this, 1, 1, 100 );
    m_MachNpts.SetDescript( "Freestream Mach number (Num Points)" );


    // Case Setup
    m_NCPU.Init( "NCPU", groupname, this, 4, 1, 255 );
    m_NCPU.SetDescript( "Number of processors to use" );

    //    wake parameters
    m_WakeNumIter.Init( "WakeNumIter", groupname, this, 5, 3, 255 );
    m_WakeNumIter.SetDescript( "Number of wake iterations to execute, Default = 5" );
    m_WakeAvgStartIter.Init( "WakeAvgStartIter", groupname, this, 0, 0, 255 );
    m_WakeAvgStartIter.SetDescript( "Iteration at which to START averaging the wake. Default=0 --> No wake averaging" );
    m_WakeSkipUntilIter.Init( "WakeSkipUntilIter", groupname, this, 0, 0, 255 );
    m_WakeSkipUntilIter.SetDescript( "Iteration at which to START calculating the wake. Default=0 --> Wake calculated on each iteration" );
    m_NumWakeNodes.SetPowShift( 2, 0 ); // Must come before Init
    m_NumWakeNodes.Init( "RootWakeNodes", groupname, this, 64, 0, 10e12 );
    m_NumWakeNodes.SetDescript( "Number of Wake Nodes (f(n^2)" );

    m_BatchModeFlag.Init( "BatchModeFlag", groupname, this, true, false, true );
    m_BatchModeFlag.SetDescript( "Flag to calculate in batch mode" );
    m_BatchModeFlag = true;

    // This sets all the filename members to the appropriate value (for example: empty strings if there is no vehicle)
    UpdateFilenames();

    m_SolverProcessKill = false;

    // Plot limits
    m_ConvergenceXMinIsManual.Init( "m_ConvergenceXMinIsManual", groupname, this, 0, 0, 1 );
    m_ConvergenceXMaxIsManual.Init( "m_ConvergenceXMaxIsManual", groupname, this, 0, 0, 1 );
    m_ConvergenceYMinIsManual.Init( "m_ConvergenceYMinIsManual", groupname, this, 0, 0, 1 );
    m_ConvergenceYMaxIsManual.Init( "m_ConvergenceYMaxIsManual", groupname, this, 0, 0, 1 );
    m_ConvergenceXMin.Init( "m_ConvergenceXMin", groupname, this, -1, -1e12, 1e12 );
    m_ConvergenceXMax.Init( "m_ConvergenceXMax", groupname, this, 1, -1e12, 1e12 );
    m_ConvergenceYMin.Init( "m_ConvergenceYMin", groupname, this, -1, -1e12, 1e12 );
    m_ConvergenceYMax.Init( "m_ConvergenceYMax", groupname, this, 1, -1e12, 1e12 );

    m_LoadDistXMinIsManual.Init( "m_LoadDistXMinIsManual", groupname, this, 0, 0, 1 );
    m_LoadDistXMaxIsManual.Init( "m_LoadDistXMaxIsManual", groupname, this, 0, 0, 1 );
    m_LoadDistYMinIsManual.Init( "m_LoadDistYMinIsManual", groupname, this, 0, 0, 1 );
    m_LoadDistYMaxIsManual.Init( "m_LoadDistYMaxIsManual", groupname, this, 0, 0, 1 );
    m_LoadDistXMin.Init( "m_LoadDistXMin", groupname, this, -1, -1e12, 1e12 );
    m_LoadDistXMax.Init( "m_LoadDistXMax", groupname, this, 1, -1e12, 1e12 );
    m_LoadDistYMin.Init( "m_LoadDistYMin", groupname, this, -1, -1e12, 1e12 );
    m_LoadDistYMax.Init( "m_LoadDistYMax", groupname, this, 1, -1e12, 1e12 );

    m_SweepXMinIsManual.Init( "m_SweepXMinIsManual", groupname, this, 0, 0, 1 );
    m_SweepXMaxIsManual.Init( "m_SweepXMaxIsManual", groupname, this, 0, 0, 1 );
    m_SweepYMinIsManual.Init( "m_SweepYMinIsManual", groupname, this, 0, 0, 1 );
    m_SweepYMaxIsManual.Init( "m_SweepYMaxIsManual", groupname, this, 0, 0, 1 );
    m_SweepXMin.Init( "m_SweepXMin", groupname, this, -1, -1e12, 1e12 );
    m_SweepXMax.Init( "m_SweepXMax", groupname, this, 1, -1e12, 1e12 );
    m_SweepYMin.Init( "m_SweepYMin", groupname, this, -1, -1e12, 1e12 );
    m_SweepYMax.Init( "m_SweepYMax", groupname, this, 1, -1e12, 1e12 );

    m_CpSliceXMinIsManual.Init( "m_CpSliceXMinIsManual", groupname, this, 0, 0, 1 );
    m_CpSliceXMaxIsManual.Init( "m_CpSliceXMaxIsManual", groupname, this, 0, 0, 1 );
    m_CpSliceYMinIsManual.Init( "m_CpSliceYMinIsManual", groupname, this, 0, 0, 1 );
    m_CpSliceYMaxIsManual.Init( "m_CpSliceYMaxIsManual", groupname, this, 0, 0, 1 );
    m_CpSliceXMin.Init( "m_CpSliceXMin", groupname, this, -1, -1e12, 1e12 );
    m_CpSliceXMax.Init( "m_CpSliceXMax", groupname, this, 1, -1e12, 1e12 );
    m_CpSliceYMin.Init( "m_CpSliceYMin", groupname, this, -1, -1e12, 1e12 );
    m_CpSliceYMax.Init( "m_CpSliceYMax", groupname, this, 1, -1e12, 1e12 );

    m_CpSliceYAxisFlipFlag.Init( "CpSliceYAxisFlipFlag", groupname, this, false, false, true );
    m_CpSliceYAxisFlipFlag.SetDescript( "Flag to Flip Y Axis in Cp Slice Plot" );
    m_CpSlicePlotLinesFlag.Init( "CpSlicePlotLinesFlag", groupname, this, true, false, true );
    m_CpSlicePlotLinesFlag.SetDescript( "Flag to Plot Lines" );

    m_UnsteadyXMinIsManual.Init( "m_UnsteadyXMinIsManual", groupname, this, 0, 0, 1 );
    m_UnsteadyXMaxIsManual.Init( "m_UnsteadyXMaxIsManual", groupname, this, 0, 0, 1 );
    m_UnsteadyYMinIsManual.Init( "m_UnsteadyYMinIsManual", groupname, this, 0, 0, 1 );
    m_UnsteadyYMaxIsManual.Init( "m_UnsteadyYMaxIsManual", groupname, this, 0, 0, 1 );
    m_UnsteadyXMin.Init( "m_UnsteadyXMin", groupname, this, -1, -1e12, 1e12 );
    m_UnsteadyXMax.Init( "m_UnsteadyXMax", groupname, this, 1, -1e12, 1e12 );
    m_UnsteadyYMin.Init( "m_UnsteadyYMin", groupname, this, -1, -1e12, 1e12 );
    m_UnsteadyYMax.Init( "m_UnsteadyYMax", groupname, this, 1, -1e12, 1e12 );

    // Other Setup Parameters
    m_Vinf.Init( "Vinf", groupname, this, 100, 0, 1e6 );
    m_Vinf.SetDescript( "Freestream Velocity Through Disk Component" );
    m_Rho.Init( "Rho", groupname, this, 0.002377, 0, 1e3 );
    m_Rho.SetDescript( "Freestream Density" );
    m_ReCref.Init( "ReCref", groupname, this, 10000000., 0, 1e12 );
    m_ReCref.SetDescript( "Reynolds Number along Reference Chord" );
    m_Precondition.Init( "Precondition", groupname, this, vsp::PRECON_MATRIX, vsp::PRECON_MATRIX, vsp::PRECON_SSOR );
    m_Precondition.SetDescript( "Preconditioner Choice" );
    m_KTCorrection.Init( "KTCorrection", groupname, this, false, false, true );
    m_KTCorrection.SetDescript( "Activate 2nd Order Karman-Tsien Mach Number Correction" );
    m_Symmetry.Init( "Symmetry", groupname, this, false, false, true );
    m_Symmetry.SetDescript( "Toggle X-Z Symmetry to Improve Calculation Time" );
    m_Write2DFEMFlag.Init( "Write2DFEMFlag", groupname, this, false, false, true );
    m_Write2DFEMFlag.SetDescript( "Toggle File Write for 2D FEM" );
    m_ClMax.Init( "Clmax", groupname, this, -1, -1, 1e3 );
    m_ClMax.SetDescript( "Cl Max of Aircraft" );
    m_ClMaxToggle.Init( "ClmaxToggle", groupname, this, false, false, true );
    m_MaxTurnAngle.Init( "MaxTurnAngle", groupname, this, -1, -1, 360 );
    m_MaxTurnAngle.SetDescript( "Max Turning Angle of Aircraft" );
    m_MaxTurnToggle.Init( "MaxTurnToggle", groupname, this, false, false, true );
    m_FarDist.Init( "FarDist", groupname, this, -1, -1, 1e6 );
    m_FarDist.SetDescript( "Far Field Distance for Wake Adaptation" );
    m_FarDistToggle.Init( "FarDistToggle", groupname, this, false, false, true );
    m_CpSliceFlag.Init( "CpSliceFlag", groupname, this, true, false, true );
    m_CpSliceFlag.SetDescript( "Flag to Calculate Cp Slices for Each Run Case" );
    m_FromSteadyState.Init( "FromSteadyState", groupname, this, false, false, true );
    m_FromSteadyState.SetDescript( "Flag to Indicate Steady State" );
    m_GroundEffect.Init( "GroundEffect", groupname, this, -1, -1, 1e6 );
    m_GroundEffect.SetDescript( "Ground Effect Distance" );
    m_GroundEffectToggle.Init( "GroundEffectToggle", groupname, this, false, false, true );

    // Unsteady
    m_StabilityCalcFlag.Init( "StabilityCalcFlag", groupname, this, false, false, true );
    m_StabilityCalcFlag.SetDescript( "Flag to calculate stability derivatives" );

    m_StabilityType.Init( "UnsteadyType", groupname, this, vsp::STABILITY_DEFAULT, vsp::STABILITY_DEFAULT, vsp::STABILITY_IMPULSE );
    m_StabilityType.SetDescript( "Unsteady Calculation Type" );

    m_CurrentCSGroupIndex = -1;
    m_CurrentRotorDiskIndex = -1;
    m_LastSelectedType = -1;
    m_CurrentCpSliceIndex = -1;

    m_Verbose = false;
}

void VSPAEROMgrSingleton::ParmChanged( Parm* parm_ptr, int type )
{
    Vehicle* veh = VehicleMgr.GetVehicle();

    if ( veh )
    {
        veh->ParmChanged( parm_ptr, type );
    }
}

void VSPAEROMgrSingleton::Renew()
{
    for(size_t i = 0; i < m_ControlSurfaceGroupVec.size(); ++i)
    {
        delete m_ControlSurfaceGroupVec[i];
        m_ControlSurfaceGroupVec.erase( m_ControlSurfaceGroupVec.begin() + i );
    }
    m_ControlSurfaceGroupVec.clear();
    m_CompleteControlSurfaceVec.clear();
    m_ActiveControlSurfaceVec.clear();

    for(size_t i = 0; i < m_RotorDiskVec.size(); ++i)
    {
        delete m_RotorDiskVec[i];
        m_RotorDiskVec.erase( m_RotorDiskVec.begin() + i );
    }
    m_RotorDiskVec.clear();

    ClearCpSliceVec();

    m_DegenGeomVec.clear();

    m_CurrentCSGroupIndex = -1;
    m_CurrentRotorDiskIndex = -1;
    m_LastSelectedType = -1;

    m_AnalysisMethod.Set( vsp::VORTEX_LATTICE );
    m_GeomSet.Set( vsp::SET_ALL );
    m_RefFlag.Set( vsp::MANUAL_REF );
    m_Sref.Set( 100 );
    m_bref.Set( 1.0 );
    m_cref.Set( 1.0 );

    m_CGGeomSet.Set( vsp::SET_ALL );
    m_NumMassSlice.Set( 10 );
    m_Xcg.Set( 0.0 );
    m_Ycg.Set( 0.0 );
    m_Zcg.Set( 0.0 );

    m_AlphaStart.Set( 1.0 ); m_AlphaEnd.Set( 10 ); m_AlphaNpts.Set( 3 );
    m_BetaStart.Set( 0.0 ); m_BetaEnd.Set( 0.0 ); m_BetaNpts.Set( 1 );
    m_MachStart.Set( 0.0 ); m_MachEnd.Set( 0.0 ); m_MachNpts.Set( 1 );

    m_BatchModeFlag.Set( true );
    m_Precondition.Set( vsp::PRECON_MATRIX );
    m_KTCorrection.Set( false );
    m_Symmetry.Set( false );
    m_StabilityCalcFlag.Set( false );
    m_StabilityType.Set( vsp::STABILITY_DEFAULT );

    m_NCPU.Set( 4 );

    m_WakeNumIter.Set( 5 );
    m_WakeAvgStartIter.Set( 0 );
    m_WakeSkipUntilIter.Set( 0 );

    m_ClMaxToggle.Set( false );
    m_MaxTurnToggle.Set( false );
    m_FarDistToggle.Set( false );
    m_GroundEffectToggle.Set( false );
    m_FromSteadyState.Set( false );
    m_NumWakeNodes.Set( 0 );
}

xmlNodePtr VSPAEROMgrSingleton::EncodeXml( xmlNodePtr & node )
{
    xmlNodePtr VSPAEROsetnode = xmlNewChild( node, NULL, BAD_CAST"VSPAEROSettings", NULL );

    ParmContainer::EncodeXml( VSPAEROsetnode ); // Encode VSPAEROMgr Parms

    // Encode Control Surface Groups using Internal Encode Method
    XmlUtil::AddIntNode( VSPAEROsetnode, "ControlSurfaceGroupCount", m_ControlSurfaceGroupVec.size() );
    for ( size_t i = 0; i < m_ControlSurfaceGroupVec.size(); ++i )
    {
        xmlNodePtr csgnode = xmlNewChild( VSPAEROsetnode, NULL, BAD_CAST "Control_Surface_Group", NULL );
        m_ControlSurfaceGroupVec[i]->EncodeXml( csgnode );
    }

    // Encode Rotor Disks using Internal Encode Method
    XmlUtil::AddIntNode( VSPAEROsetnode, "RotorDiskCount", m_RotorDiskVec.size() );
    for ( size_t i = 0; i < m_RotorDiskVec.size(); ++i )
    {
        xmlNodePtr rotornode = xmlNewChild( VSPAEROsetnode, NULL, BAD_CAST "Rotor", NULL );
        m_RotorDiskVec[i]->EncodeXml( rotornode );
    }

    // Encode CpSlices using Internal Encode Method
    XmlUtil::AddIntNode( VSPAEROsetnode, "CpSliceCount", m_CpSliceVec.size() );
    for ( size_t i = 0; i < m_CpSliceVec.size(); ++i )
    {
        xmlNodePtr cpslicenode = xmlNewChild( VSPAEROsetnode, NULL, BAD_CAST "CpSlice", NULL );
        m_CpSliceVec[i]->EncodeXml( cpslicenode );
    }

    return VSPAEROsetnode;
}

xmlNodePtr VSPAEROMgrSingleton::DecodeXml( xmlNodePtr & node )
{
    xmlNodePtr VSPAEROsetnode = XmlUtil::GetNode( node, "VSPAEROSettings", 0 );
    if ( VSPAEROsetnode )
    {
        ParmContainer::DecodeXml( VSPAEROsetnode ); // Decode VSPAEROMgr Parms

        // Decode Control Surface Groups using Internal Decode Method
        int num_groups = XmlUtil::FindInt( VSPAEROsetnode, "ControlSurfaceGroupCount", 0 );
        for ( size_t i = 0; i < num_groups; ++i )
        {
            xmlNodePtr csgnode = XmlUtil::GetNode( VSPAEROsetnode, "Control_Surface_Group", i );
            if ( csgnode )
            {
                AddControlSurfaceGroup();
                m_ControlSurfaceGroupVec.back()->DecodeXml( csgnode );
            }
        }

        // Decode Rotor Disks using Internal Decode Method
        int num_rotor = XmlUtil::FindInt( VSPAEROsetnode, "RotorDiskCount", 0 );
        for ( size_t i = 0; i < num_rotor; ++i )
        {
            xmlNodePtr rotornode = XmlUtil::GetNode( VSPAEROsetnode, "Rotor", i );
            if ( rotornode )
            {
                AddRotorDisk();
                m_RotorDiskVec.back()->DecodeXml( rotornode );
            }
        }

        // Decode CpSlices using Internal Decode Method
        int num_slice = XmlUtil::FindInt( VSPAEROsetnode, "CpSliceCount", 0 );
        for ( size_t i = 0; i < num_slice; ++i )
        {
            xmlNodePtr cpslicenode = XmlUtil::GetNode( VSPAEROsetnode, "CpSlice", i );
            if ( cpslicenode )
            {
                AddCpSlice();
                m_CpSliceVec.back()->DecodeXml( cpslicenode );
            }
        }
    }

    UpdateControlSurfaceGroupSuffix();
    UpdateRotorDiskSuffix();

    return VSPAEROsetnode;
}


void VSPAEROMgrSingleton::Update()
{
    UpdateSref();

    UpdateFilenames();

    UpdateRotorDisks();

    UpdateCompleteControlSurfVec();

    UpdateControlSurfaceGroups();

    UpdateActiveControlSurfVec();

    UpdateSetupParmLimits();
}

void VSPAEROMgrSingleton::UpdateSref()
{
    if( m_RefFlag() == vsp::MANUAL_REF )
    {
        m_Sref.Activate();
        m_bref.Activate();
        m_cref.Activate();
    }
    else
    {
        Geom* refgeom = VehicleMgr.GetVehicle()->FindGeom( m_RefGeomID );

        if( refgeom )
        {
            if( refgeom->GetType().m_Type == MS_WING_GEOM_TYPE )
            {
                WingGeom* refwing = ( WingGeom* ) refgeom;
                m_Sref.Set( refwing->m_TotalArea() );
                m_bref.Set( refwing->m_TotalSpan() );
                m_cref.Set( refwing->m_TotalChord() );

                m_Sref.Deactivate();
                m_bref.Deactivate();
                m_cref.Deactivate();
            }
        }
        else
        {
            m_RefGeomID = string();
        }
    }
}

void VSPAEROMgrSingleton::UpdateSetupParmLimits()
{
    if ( m_ClMaxToggle() )
    {
        m_ClMax.SetLowerLimit( 0.0 );
        m_ClMax.Activate();
    }
    else
    {
        m_ClMax.SetLowerLimit( -1.0 );
        m_ClMax.Set( -1.0 );
        m_ClMax.Deactivate();
    }

    if ( m_MaxTurnToggle() )
    {
        m_MaxTurnAngle.SetLowerLimit( 0.0 );
        m_MaxTurnAngle.Activate();
    }
    else
    {
        m_MaxTurnAngle.SetLowerLimit( -1.0 );
        m_MaxTurnAngle.Set( -1.0 );
        m_MaxTurnAngle.Deactivate();
    }

    if ( m_FarDistToggle() )
    {
        m_FarDist.SetLowerLimit( 0.0 );
        m_FarDist.Activate();
    }
    else
    {
        m_FarDist.SetLowerLimit( -1.0 );
        m_FarDist.Set( -1.0 );
        m_FarDist.Deactivate();
    }

    if ( m_GroundEffectToggle() )
    {
        m_GroundEffect.SetLowerLimit( 0.0 );
        m_GroundEffect.Activate();
    }
    else
    {
        m_GroundEffect.SetLowerLimit( -1.0 );
        m_GroundEffect.Set( -1.0 );
        m_GroundEffect.Deactivate();
    }
}

void VSPAEROMgrSingleton::UpdateFilenames()    //A.K.A. SetupDegenFile()
{
    // Initialize these to blanks.  if any of the checks fail the variables will at least contain an empty string
    m_ModelNameBase     = string();
    m_DegenFileFull     = string();
    m_CompGeomFileFull  = string();     // TODO this is set from the get export name
    m_SetupFile         = string();
    m_AdbFile           = string();
    m_HistoryFile       = string();
    m_LoadFile          = string();
    m_StabFile          = string();
    m_CutsFile          = string();
    m_SliceFile         = string();

    Vehicle *veh = VehicleMgr.GetVehicle();
    if( veh )
    {
        // Generate the base name based on the vsp3filename without the extension
        int pos = -1;
        switch ( m_AnalysisMethod.Get() )
        {
        case vsp::VORTEX_LATTICE:
            // The base_name is dependent on the DegenFileName
            // TODO extra "_DegenGeom" is added to the m_ModelBase
            m_DegenFileFull = veh->getExportFileName( vsp::DEGEN_GEOM_CSV_TYPE );

            m_ModelNameBase = m_DegenFileFull;
            pos = m_ModelNameBase.find( ".csv" );
            if ( pos >= 0 )
            {
                m_ModelNameBase.erase( pos, m_ModelNameBase.length() - 1 );
            }

            m_CompGeomFileFull  = string(); //This file is not used for vortex lattice analysis
            m_SetupFile         = m_ModelNameBase + string( ".vspaero" );
            m_AdbFile           = m_ModelNameBase + string( ".adb" );
            m_HistoryFile       = m_ModelNameBase + string( ".history" );
            m_LoadFile          = m_ModelNameBase + string( ".lod" );
            m_StabFile          = m_ModelNameBase + string( ".stab" );
            m_CutsFile          = m_ModelNameBase + string( ".cuts" );
            m_SliceFile         = m_ModelNameBase + string( ".slc" );

            break;

        case vsp::PANEL:
            m_CompGeomFileFull = veh->getExportFileName( vsp::VSPAERO_PANEL_TRI_TYPE );

            m_ModelNameBase = m_CompGeomFileFull;
            pos = m_ModelNameBase.find( ".tri" );
            if ( pos >= 0 )
            {
                m_ModelNameBase.erase( pos, m_ModelNameBase.length() - 1 );
            }

            m_DegenFileFull     = m_ModelNameBase + string( "_DegenGeom.csv" );
            m_SetupFile         = m_ModelNameBase + string( ".vspaero" );
            m_AdbFile           = m_ModelNameBase + string( ".adb" );
            m_HistoryFile       = m_ModelNameBase + string( ".history" );
            m_LoadFile          = m_ModelNameBase + string( ".lod" );
            m_StabFile          = m_ModelNameBase + string( ".stab" );
            m_CutsFile          = m_ModelNameBase + string( ".cuts" );
            m_SliceFile         = m_ModelNameBase + string( ".slc" );

            break;

        default:
            // TODO print out an error here
            break;
        }
    }
}

void VSPAEROMgrSingleton::UpdateRotorDisks()
{
    Vehicle * veh = VehicleMgr.GetVehicle();
    char str[256];

    if ( veh )
    {
        vector < RotorDisk* > temp;
        bool contained = false;

        vector <string> currgeomvec = veh->GetGeomSet( m_GeomSet() );

        for ( size_t i = 0; i < currgeomvec.size(); ++i )
        {
            Geom* geom = veh->FindGeom(currgeomvec[i]);
            if (geom)
            {
                vector < VspSurf > surfvec;
                geom->GetSurfVec(surfvec);
                for (size_t iSubsurf = 0; iSubsurf < geom->GetNumTotalSurfs(); ++iSubsurf)
                {
                    contained = false;
                    if (surfvec[iSubsurf].GetSurfType() == vsp::DISK_SURF)
                    {
                        for (size_t j = 0; j < m_RotorDiskVec.size(); ++j)
                        {
                            // If Rotor Disk and Corresponding Surface Num Already Exists within m_RotorDiskVec
                            if (m_RotorDiskVec[j]->m_ParentGeomId == currgeomvec[i] && m_RotorDiskVec[j]->GetSurfNum() == iSubsurf)
                            {
                                contained = true;
                                temp.push_back(m_RotorDiskVec[j]);
                                for (size_t k = 0; k < m_DegenGeomVec.size(); ++k)
                                {
                                    if (m_DegenGeomVec[k].getParentGeom()->GetID().compare(m_RotorDiskVec[j]->m_ParentGeomId) == 0)
                                    {
                                        int indxToSearch = k + temp.back()->m_ParentGeomSurfNdx;
                                        temp.back()->m_XYZ = m_DegenGeomVec[indxToSearch].getDegenDisk().x;
                                        temp.back()->m_Normal = m_DegenGeomVec[indxToSearch].getDegenDisk().nvec * -1.0;
                                        break;
                                    }
                                }
                                sprintf(str, "%s_%u", geom->GetName().c_str(), iSubsurf);
                                temp.back()->SetName(str);
                            }
                        }

                        // If Rotor Disk and Corresponding Surface Num Do NOT Exist within m_RotorDiskVec
                        // Create New Rotor Disk Parm Container
                        if (!contained)
                        {
                            RotorDisk *rotor = new RotorDisk();
                            temp.push_back(rotor);
                            temp.back()->m_ParentGeomId = currgeomvec[i];
                            temp.back()->m_ParentGeomSurfNdx = iSubsurf;
                            sprintf(str, "%s_%u", geom->GetName().c_str(), iSubsurf);
                            temp.back()->SetName(str);
                        }

                        string dia_id = geom->FindParm("Diameter", "Design");
                        temp.back()->m_Diameter.Set(ParmMgr.FindParm(dia_id)->Get());
                        if (temp.back()->m_HubDiameter() > temp.back()->m_Diameter())
                        {
                            temp.back()->m_HubDiameter.Set(temp.back()->m_Diameter());
                        }
                    }
                }
            }
        }

        // Check for and delete any disks that no longer exist
        for ( size_t i = 0; i < m_RotorDiskVec.size(); ++i )
        {
            bool delete_flag = true;
            for ( size_t j = 0; j < temp.size(); ++j )
            {
                if ( temp[j] == m_RotorDiskVec[i] )
                {
                    delete_flag = false;
                }
            }
            if ( delete_flag )
            {
                delete m_RotorDiskVec[i];
                m_RotorDiskVec.erase( m_RotorDiskVec.begin() + i );
            }
        }
        m_RotorDiskVec.clear();
        m_RotorDiskVec = temp;
    }

    UpdateRotorDiskSuffix();
}

void VSPAEROMgrSingleton::UpdateControlSurfaceGroups()
{
    for ( size_t i = 0; i < m_ControlSurfaceGroupVec.size(); ++i )
    {
        for ( size_t k = 0; k < m_ControlSurfaceGroupVec[i]->m_ControlSurfVec.size(); ++k )
        {
            for ( size_t j = 0; j < m_CompleteControlSurfaceVec.size(); ++j )
            {
                // If Control Surface ID AND Reflection Number Match - Replace Subsurf within Control Surface Group
                if ( m_ControlSurfaceGroupVec[i]->m_ControlSurfVec[k].SSID.compare( m_CompleteControlSurfaceVec[j].SSID ) == 0 &&
                        m_ControlSurfaceGroupVec[i]->m_ControlSurfVec[k].iReflect == m_CompleteControlSurfaceVec[j].iReflect )
                {
                    m_ControlSurfaceGroupVec[i]->m_ControlSurfVec[k].fullName = m_CompleteControlSurfaceVec[j].fullName;
                    m_CompleteControlSurfaceVec[j].isGrouped = true;
                    m_ControlSurfaceGroupVec[i]->m_ControlSurfVec[k].isGrouped = true;
                }
            }
            // Remove Deleted Sub Surfaces and Sub Surfaces with Parent Geoms That No Longer Exist
            Geom* parent = VehicleMgr.GetVehicle()->FindGeom( m_ControlSurfaceGroupVec[i]->m_ControlSurfVec[k].parentGeomId );
            if ( !parent || !parent->GetSubSurf( m_ControlSurfaceGroupVec[i]->m_ControlSurfVec[k].SSID ) )
            {
                m_ControlSurfaceGroupVec[i]->RemoveSubSurface( m_ControlSurfaceGroupVec[i]->m_ControlSurfVec[k].SSID,
                        m_ControlSurfaceGroupVec[i]->m_ControlSurfVec[k].iReflect );
                k--;
            }
        }
    }
    UpdateControlSurfaceGroupSuffix();
}

void VSPAEROMgrSingleton::CleanCompleteControlSurfVec()
{
    Vehicle* veh = VehicleMgr.GetVehicle();

    if ( veh )
    {
        // Clean Out No Longer Existing Control Surfaces (Due to Geom Changes)
        for ( size_t i = 0; i < m_CompleteControlSurfaceVec.size(); ++i )
        {
            Geom* geom = veh->FindGeom( m_CompleteControlSurfaceVec[i].parentGeomId );
            if ( !geom )
            {
                m_CompleteControlSurfaceVec.erase( m_CompleteControlSurfaceVec.begin() + i );
                --i;
            }
            else if ( !geom->GetSubSurf( m_CompleteControlSurfaceVec[i].SSID ) )
            {
                m_CompleteControlSurfaceVec.erase( m_CompleteControlSurfaceVec.begin() + i );
                --i;
            }
        }
    }
}

void VSPAEROMgrSingleton::UpdateCompleteControlSurfVec()
{
    m_CompleteControlSurfaceVec.clear();

    Vehicle* veh = VehicleMgr.GetVehicle();
    if ( veh )
    {
        vector< string > geom_vec = veh->GetGeomVec();
        for ( size_t i = 0; i < geom_vec.size(); ++i )
        {
            Geom *g = veh->FindGeom( geom_vec[i] );
            if ( g )
            {
                vector < SubSurface* > sub_surf_vec = g->GetSubSurfVec();
                for ( size_t j = 0; j < sub_surf_vec.size(); ++j )
                {
                    SubSurface *ssurf = sub_surf_vec[j];
                    if ( ssurf )
                    {
                        for ( size_t iReflect = 0; iReflect < g->GetNumSymmCopies(); ++iReflect )
                        {
                            if ( ssurf->GetType() == vsp::SS_CONTROL || ssurf->GetType() == vsp::SS_RECTANGLE )
                            {
                                // Create New CS Parm Container
                                VspAeroControlSurf newSurf;
                                newSurf.SSID = ssurf->GetID();
                                char str[256];
                                sprintf( str, "%s_Surf%u_%s", g->GetName().c_str(), iReflect, ssurf->GetName().c_str() );
                                newSurf.fullName = string( str );
                                newSurf.parentGeomId = ssurf->GetParentContainer();
                                newSurf.iReflect = iReflect;

                                m_CompleteControlSurfaceVec.push_back( newSurf );
                            }
                        }
                    }
                }
            }
        }

        CleanCompleteControlSurfVec();
    }
}

void VSPAEROMgrSingleton::UpdateActiveControlSurfVec()
{
    m_ActiveControlSurfaceVec.clear();
    if ( m_CurrentCSGroupIndex != -1 )
    {
        vector < VspAeroControlSurf > sub_surf_vec = m_ControlSurfaceGroupVec[ m_CurrentCSGroupIndex ]->m_ControlSurfVec;
        for ( size_t j = 0; j < sub_surf_vec.size(); ++j )
        {
            m_ActiveControlSurfaceVec.push_back( sub_surf_vec[j] );
        }
    }
}

void VSPAEROMgrSingleton::AddLinkableParms( vector < string > & linkable_parm_vec, const string & link_container_id )
{
    ParmContainer::AddLinkableParms( linkable_parm_vec );

    for ( size_t i = 0; i < m_ControlSurfaceGroupVec.size(); ++i )
    {
        m_ControlSurfaceGroupVec[i]->AddLinkableParms( linkable_parm_vec, m_ID );
    }

    for ( size_t i = 0; i < m_RotorDiskVec.size(); ++i )
    {
        m_RotorDiskVec[i]->AddLinkableParms( linkable_parm_vec, m_ID );
    }

    for ( size_t i = 0; i < m_CpSliceVec.size(); ++i )
    {
        m_CpSliceVec[i]->AddLinkableParms( linkable_parm_vec, m_ID );
    }
}

// InitControlSurfaceGroups - creates the initial default grouping for the control surfaces
//  The initial grouping collects all surface copies of the subsurface into a single group.
//  For example if a wing is defined with an aileron and that wing is symetrical about the
//  xz plane there will be a surface copy of the master wing surface as well as a copy of
//  the subsurface. The two subsurfaces may get deflected differently during analysis
//  routines and can be identified uniquely by the control_surf.fullname.
//  The initial grouping routine implemented here finds all copies of that subsurface
//  that have the same sub surf ID and places them into a single control group.
void VSPAEROMgrSingleton::InitControlSurfaceGroups()
{
    Vehicle * veh = VehicleMgr.GetVehicle();
    if ( !veh )
    {
        return;
    }

    ControlSurfaceGroup * csg;
    char str [256];

    for ( size_t i = 0 ; i < m_CompleteControlSurfaceVec.size(); ++i )
    {
        // Only Autogroup Ungrouped Control Surfaces
        if ( !m_CompleteControlSurfaceVec[i].isGrouped )
        {
            bool exists = false;

            // Has CS been placed into init group?
            // --> No create group with any reflected groups
            // --> Yes Skip
            for ( size_t j = 0; j < m_ControlSurfaceGroupVec.size(); ++j )
            {
                // Check if the control surface is available
                m_CurrentCSGroupIndex = j;
                UpdateActiveControlSurfVec();
                vector < VspAeroControlSurf > ungrouped_vec = GetAvailableCSVec();
                bool is_available = false;

                for ( size_t k = 0; k < ungrouped_vec.size(); k++ )
                {
                    if ( ( m_CompleteControlSurfaceVec[i].fullName == ungrouped_vec[k].fullName ) &&
                        ( m_CompleteControlSurfaceVec[i].parentGeomId == ungrouped_vec[k].parentGeomId ) &&
                        ( m_CompleteControlSurfaceVec[i].SSID == ungrouped_vec[k].SSID ) &&
                        ( m_CompleteControlSurfaceVec[i].isGrouped == ungrouped_vec[k].isGrouped ) &&
                        ( m_CompleteControlSurfaceVec[i].iReflect == ungrouped_vec[k].iReflect ) )
                    {
                        is_available = true;
                        break;
                    }
                }

                if ( m_ControlSurfaceGroupVec[j]->m_ControlSurfVec.size() > 0 && is_available )
                {
                    // Construct a default group name
                    string curr_csg_id = m_CompleteControlSurfaceVec[i].parentGeomId + "_" + m_CompleteControlSurfaceVec[i].SSID;

                    sprintf( str, "%s_%s", m_ControlSurfaceGroupVec[j]->m_ParentGeomBaseID.c_str(),
                        m_ControlSurfaceGroupVec[j]->m_ControlSurfVec[0].SSID.c_str() );
                    if ( curr_csg_id == str ) // Update Existing Control Surface Group
                    {
                        csg = m_ControlSurfaceGroupVec[j];
                        csg->AddSubSurface( m_CompleteControlSurfaceVec[i] );
                        m_ControlSurfaceGroupVec.back() = csg;
                        exists = true;
                        break;
                    }
                }
            }

            if ( !exists ) // Create New Control Surface Group
            {
                Geom* geom = veh->FindGeom( m_CompleteControlSurfaceVec[i].parentGeomId );
                if ( geom )
                {
                    csg = new ControlSurfaceGroup;
                    csg->AddSubSurface( m_CompleteControlSurfaceVec[i] );
                    sprintf( str, "%s_%s", geom->GetName().c_str(),
                        geom->GetSubSurf( m_CompleteControlSurfaceVec[i].SSID )->GetName().c_str() );
                    csg->SetName( str );
                    csg->m_ParentGeomBaseID = m_CompleteControlSurfaceVec[i].parentGeomId;
                    m_ControlSurfaceGroupVec.push_back( csg );
                }
            }
        }
    }

    UpdateControlSurfaceGroupSuffix();
}

string VSPAEROMgrSingleton::ComputeGeometry()
{
    Vehicle *veh = VehicleMgr.GetVehicle();
    if ( !veh )
    {
        fprintf( stderr, "ERROR: Unable to get vehicle \n\tFile: %s \tLine:%d\n", __FILE__, __LINE__ );
        return string();
    }

    // Cleanup previously created meshGeom IDs created from VSPAEROMgr
    if ( veh->FindGeom( m_LastPanelMeshGeomId ) )
    {
        veh->DeleteGeom( m_LastPanelMeshGeomId );
        if ( m_AnalysisMethod() == vsp::VORTEX_LATTICE )
        {
            veh->ShowOnlySet( m_GeomSet() );
        }
    }

    m_DegenGeomVec.clear();
    veh->CreateDegenGeom( m_GeomSet() );
    m_DegenGeomVec = veh->GetDegenGeomVec();

    //Update information derived from the degenerate geometry
    UpdateRotorDisks();
    UpdateCompleteControlSurfVec();

    // record original values
    bool exptMfile_orig = veh->getExportDegenGeomMFile();
    bool exptCSVfile_orig = veh->getExportDegenGeomCsvFile();
    veh->setExportDegenGeomMFile( false );
    veh->setExportDegenGeomCsvFile( true );

    UpdateFilenames();

    // Note: while in panel mode the degen file required by vspaero is
    // dependent on the tri filename and not necessarily what the current
    // setting is for the vsp::DEGEN_GEOM_CSV_TYPE
    string degenGeomFile_orig = veh->getExportFileName( vsp::DEGEN_GEOM_CSV_TYPE );
    veh->setExportFileName( vsp::DEGEN_GEOM_CSV_TYPE, m_DegenFileFull );

    veh->WriteDegenGeomFile();

    // restore original values
    veh->setExportDegenGeomMFile( exptMfile_orig );
    veh->setExportDegenGeomCsvFile( exptCSVfile_orig );
    veh->setExportFileName( vsp::DEGEN_GEOM_CSV_TYPE, degenGeomFile_orig );

    WaitForFile( m_DegenFileFull );
    if ( !FileExist( m_DegenFileFull ) )
    {
        fprintf( stderr, "WARNING: DegenGeom file not found: %s\n\tFile: %s \tLine:%d\n", m_DegenFileFull.c_str(), __FILE__, __LINE__ );
    }

    // Generate *.tri geometry file for Panel method
    if ( m_AnalysisMethod.Get() == vsp::PANEL )
    {
        // Compute intersected and trimmed geometry
        int halfFlag = 0;

        if ( m_Symmetry() )
        {
            halfFlag = 1;
        }

        m_LastPanelMeshGeomId = veh->CompGeomAndFlatten( m_GeomSet(), halfFlag );

        // After CompGeomAndFlatten() is run all the geometry is hidden and the intersected & trimmed mesh is the only one shown
        veh->WriteTRIFile( m_CompGeomFileFull , vsp::SET_SHOWN );
        WaitForFile( m_CompGeomFileFull );
        if ( !FileExist( m_CompGeomFileFull ) )
        {
            fprintf( stderr, "WARNING: CompGeom file not found: %s\n\tFile: %s \tLine:%d\n", m_CompGeomFileFull.c_str(), __FILE__, __LINE__ );
        }

    }

    // Clear previous results
    while ( ResultsMgr.GetNumResults( "VSPAERO_Geom" ) > 0 )
    {
        ResultsMgr.DeleteResult( ResultsMgr.FindResultsID( "VSPAERO_Geom",  0 ) );
    }
    // Write out new results
    Results* res = ResultsMgr.CreateResults( "VSPAERO_Geom" );
    if ( !res )
    {
        fprintf( stderr, "ERROR: Unable to create result in result manager \n\tFile: %s \tLine:%d\n", __FILE__, __LINE__ );
        return string();
    }
    res->Add( NameValData( "GeometrySet", m_GeomSet() ) );
    res->Add( NameValData( "AnalysisMethod", m_AnalysisMethod.Get() ) );
    res->Add( NameValData( "DegenGeomFileName", m_DegenFileFull ) );
    if ( m_AnalysisMethod.Get() == vsp::PANEL )
    {
        res->Add( NameValData( "CompGeomFileName", m_CompGeomFileFull ) );
        res->Add( NameValData( "Mesh_GeomID", m_LastPanelMeshGeomId ) );
    }
    else
    {
        res->Add( NameValData( "CompGeomFileName", string() ) );
        res->Add( NameValData( "Mesh_GeomID", string() ) );
    }

    return res->GetID();

}

string VSPAEROMgrSingleton::CreateSetupFile()
{
    string retStr = string();

    Update(); // Ensure correct control surface and rotor groups when this function is called through the API

    Vehicle *veh = VehicleMgr.GetVehicle();
    if ( !veh )
    {
        fprintf( stderr, "ERROR %d: Unable to get vehicle \n\tFile: %s \tLine:%d\n", vsp::VSP_INVALID_PTR, __FILE__, __LINE__ );
        return retStr;
    }

    // Clear existing setup file
    if ( FileExist( m_SetupFile ) )
    {
        remove( m_SetupFile.c_str() );
    }


    FILE * case_file = fopen( m_SetupFile.c_str(), "w" );
    if ( case_file == NULL )
    {
        fprintf( stderr, "ERROR %d: Unable to create case file: %s\n\tFile: %s \tLine:%d\n", vsp::VSP_INVALID_PTR, m_SetupFile.c_str(), __FILE__, __LINE__ );
        return retStr;
    }
    fprintf( case_file, "Sref = %lf \n", m_Sref() );
    fprintf( case_file, "Cref = %lf \n", m_cref() );
    fprintf( case_file, "Bref = %lf \n", m_bref() );
    fprintf( case_file, "X_cg = %lf \n", m_Xcg() );
    fprintf( case_file, "Y_cg = %lf \n", m_Ycg() );
    fprintf( case_file, "Z_cg = %lf \n", m_Zcg() );

    vector<double> alphaVec;
    vector<double> betaVec;
    vector<double> machVec;
    GetSweepVectors( alphaVec, betaVec, machVec );

    if ( !m_BatchModeFlag.Get() )
    {
        //truncate the vectors to just the first element
        machVec.resize( 1 );
        alphaVec.resize( 1 );
        betaVec.resize( 1 );
    }
    unsigned int i;
    // Mach vector
    fprintf( case_file, "Mach = " );
    for ( i = 0; i < machVec.size() - 1; i++ )
    {
        fprintf( case_file, "%lf, ", machVec[i] );
    }
    fprintf( case_file, "%lf \n", machVec[i++] );

    // Alpha vector
    fprintf( case_file, "AoA = " );
    for ( i = 0; i < alphaVec.size() - 1; i++ )
    {
        fprintf( case_file, "%lf, ", alphaVec[i] );
    }
    fprintf( case_file, "%lf \n", alphaVec[i++] );

    // Beta vector
    fprintf( case_file, "Beta = " );
    for ( i = 0; i < betaVec.size() - 1; i++ )
    {
        fprintf( case_file, "%lf, ", betaVec[i] );
    }
    fprintf( case_file, "%lf \n", betaVec[i++] );

    string sym;
    if ( m_Symmetry() )
    { sym = "Y"; }
    else
    { sym = "NO"; }
    fprintf( case_file, "Vinf = %lf \n", m_Vinf() );
    fprintf( case_file, "Rho = %lf \n", m_Rho() );
    fprintf( case_file, "ReCref = %lf \n", m_ReCref() );
    fprintf( case_file, "ClMax = %lf \n", m_ClMax() );
    fprintf( case_file, "MaxTurningAngle = %lf \n", m_MaxTurnAngle() );
    fprintf( case_file, "Symmetry = %s \n", sym.c_str() );
    fprintf( case_file, "FarDist = %lf \n", m_FarDist() );
    fprintf( case_file, "NumWakeNodes = %d \n", m_NumWakeNodes() );
    fprintf( case_file, "WakeIters = %d \n", m_WakeNumIter.Get() );

    // RotorDisks
    unsigned int numUsedRotors = 0;
    for ( unsigned int iRotor = 0; iRotor < m_RotorDiskVec.size(); iRotor++ )
    {
        if ( m_RotorDiskVec[iRotor]->m_IsUsed )
        {
            numUsedRotors++;
        }
    }
    fprintf( case_file, "NumberOfRotors = %d \n", numUsedRotors );           //TODO add to VSPAEROMgr as parm
    int iPropElement = 0;
    for ( unsigned int iRotor = 0; iRotor < m_RotorDiskVec.size(); iRotor++ )
    {
        if ( m_RotorDiskVec[iRotor]->m_IsUsed )
        {
            iPropElement++;
            fprintf( case_file, "PropElement_%d\n", iPropElement );     //read in by, but not used, in vspaero and begins at 1
            fprintf( case_file, "%d\n", iPropElement );                 //read in by, but not used, in vspaero
            m_RotorDiskVec[iRotor]->Write_STP_Data( case_file );
        }
    }

    // ControlSurfaceGroups
    unsigned int numUsedCSGs = 0;
    for ( size_t iCSG = 0; iCSG < m_ControlSurfaceGroupVec.size(); iCSG++ )
    {
        if ( m_ControlSurfaceGroupVec[iCSG]->m_IsUsed() && m_ControlSurfaceGroupVec[iCSG]->m_ControlSurfVec.size() > 0 )
        {
            // Don't "use" if no control surfaces are assigned to the group
            numUsedCSGs++;
        }
    }

    if ( m_AnalysisMethod.Get() == vsp::PANEL )
    {
        // control surfaces are currently not supported for panel method
        numUsedCSGs = 0;
        fprintf( case_file, "NumberOfControlGroups = %d \n", numUsedCSGs );
    }
    else
    {
        fprintf( case_file, "NumberOfControlGroups = %d \n", numUsedCSGs );
        for ( size_t iCSG = 0; iCSG < m_ControlSurfaceGroupVec.size(); iCSG++ )
        {
            if ( m_ControlSurfaceGroupVec[iCSG]->m_IsUsed() && m_ControlSurfaceGroupVec[iCSG]->m_ControlSurfVec.size() > 0 )
            {
                m_ControlSurfaceGroupVec[iCSG]->Write_STP_Data( case_file );
            }
        }
    }

    // Preconditioner (not read from setup file)
    string precon;
    if ( m_Precondition() == vsp::PRECON_MATRIX )
    {
        precon = "Matrix";
    }
    else if ( m_Precondition() == vsp::PRECON_JACOBI )
    {
        precon = "Jacobi";
    }
    else if ( m_Precondition() == vsp::PRECON_SSOR )
    {
        precon = "SSOR";
    }
    fprintf( case_file, "Preconditioner = %s \n", precon.c_str() );

    // 2nd Order Karman-Tsien Mach Number Correction (not read from setup file)
    string ktcorrect;
    if ( m_KTCorrection() )
    {
        ktcorrect = "Y";
    }
    else
    {
        ktcorrect = "N";
    }
    fprintf( case_file, "Karman-Tsien Correction = %s \n", ktcorrect.c_str() );

    //Finish up by closing the file and making sure that it appears in the file system
    fclose( case_file );

    // Wait until the setup file shows up on the file system
    WaitForFile( m_SetupFile );

    // Add and return a result
    Results* res = ResultsMgr.CreateResults( "VSPAERO_Setup" );

    if ( !FileExist( m_SetupFile ) )
    {
        // shouldn't be able to get here but create a setup file with the correct settings
        fprintf( stderr, "ERROR %d: setup file not found, file %s\n\tFile: %s \tLine:%d\n", vsp::VSP_FILE_DOES_NOT_EXIST, m_SetupFile.c_str(), __FILE__, __LINE__ );
        retStr = string();
    }
    else if ( !res )
    {
        fprintf( stderr, "ERROR: Unable to create result in result manager \n\tFile: %s \tLine:%d\n", __FILE__, __LINE__ );
        retStr = string();
    }
    else
    {
        res->Add( NameValData( "SetupFile", m_SetupFile ) );
        retStr = res->GetID();
    }

    // Send the message to update the screens
    MessageData data;
    data.m_String = "UpdateAllScreens";
    MessageMgr::getInstance().Send( "ScreenMgr", NULL, data );

    return retStr;

}

void VSPAEROMgrSingleton::ClearAllPreviousResults()
{
    while ( ResultsMgr.GetNumResults( "VSPAERO_History" ) > 0 )
    {
        ResultsMgr.DeleteResult( ResultsMgr.FindResultsID( "VSPAERO_History",  0 ) );
    }
    while ( ResultsMgr.GetNumResults( "VSPAERO_Load" ) > 0 )
    {
        ResultsMgr.DeleteResult( ResultsMgr.FindResultsID( "VSPAERO_Load",  0 ) );
    }
    while ( ResultsMgr.GetNumResults( "VSPAERO_Stab" ) > 0 )
    {
        ResultsMgr.DeleteResult( ResultsMgr.FindResultsID( "VSPAERO_Stab",  0 ) );
    }
    while ( ResultsMgr.GetNumResults( "VSPAERO_Wrapper" ) > 0 )
    {
        ResultsMgr.DeleteResult( ResultsMgr.FindResultsID( "VSPAERO_Wrapper",  0 ) );
    }
}

void VSPAEROMgrSingleton::GetSweepVectors( vector<double> &alphaVec, vector<double> &betaVec, vector<double> &machVec )
{
    // grab current parm values
    double alphaStart = m_AlphaStart.Get();
    double alphaEnd = m_AlphaEnd.Get();
    int alphaNpts = m_AlphaNpts.Get();

    double betaStart = m_BetaStart.Get();
    double betaEnd = m_BetaEnd.Get();
    int betaNpts = m_BetaNpts.Get();

    double machStart = m_MachStart.Get();
    double machEnd = m_MachEnd.Get();
    int machNpts = m_MachNpts.Get();

    // Calculate spacing
    double alphaDelta = 0.0;
    if ( alphaNpts > 1 )
    {
        alphaDelta = ( alphaEnd - alphaStart ) / ( alphaNpts - 1.0 );
    }
    for ( int iAlpha = 0; iAlpha < alphaNpts; iAlpha++ )
    {
        //Set current alpha value
        alphaVec.push_back( alphaStart + double( iAlpha ) * alphaDelta );
    }

    double betaDelta = 0.0;
    if ( betaNpts > 1 )
    {
        betaDelta = ( betaEnd - betaStart ) / ( betaNpts - 1.0 );
    }
    for ( int iBeta = 0; iBeta < betaNpts; iBeta++ )
    {
        //Set current alpha value
        betaVec.push_back( betaStart + double( iBeta ) * betaDelta );
    }

    double machDelta = 0.0;
    if ( machNpts > 1 )
    {
        machDelta = ( machEnd - machStart ) / ( machNpts - 1.0 );
    }
    for ( int iMach = 0; iMach < machNpts; iMach++ )
    {
        //Set current alpha value
        machVec.push_back( machStart + double( iMach ) * machDelta );
    }
}

/* ComputeSolver(FILE * logFile)
Returns a result with a vector of results id's under the name ResultVec
Optional input of logFile allows outputting to a log file or the console
*/
string VSPAEROMgrSingleton::ComputeSolver( FILE * logFile )
{
    UpdateFilenames();
    if ( m_CpSliceFlag() )
    {
        ClearCpSliceResults();
    }
    if ( m_BatchModeFlag.Get() )
    {
        return ComputeSolverBatch( logFile );
    }
    else
    {
        return ComputeSolverSingle( logFile );
    }

    return string();
}

/* ComputeSolverSingle(FILE * logFile)
*/
string VSPAEROMgrSingleton::ComputeSolverSingle( FILE * logFile )
{
    std::vector <string> res_id_vector;

    Vehicle *veh = VehicleMgr.GetVehicle();

    if ( veh )
    {

        string adbFileName = m_AdbFile;
        string historyFileName = m_HistoryFile;
        string loadFileName = m_LoadFile;
        string stabFileName = m_StabFile;
        string modelNameBase = m_ModelNameBase;

        bool stabilityFlag = m_StabilityCalcFlag.Get();
        vsp::VSPAERO_ANALYSIS_METHOD analysisMethod = ( vsp::VSPAERO_ANALYSIS_METHOD )m_AnalysisMethod.Get();
        vsp::VSPAERO_STABILITY_TYPE stabilityType = ( vsp::VSPAERO_STABILITY_TYPE )m_StabilityType.Get();

        int ncpu = m_NCPU.Get();

        int wakeAvgStartIter = m_WakeAvgStartIter.Get();
        int wakeSkipUntilIter = m_WakeSkipUntilIter.Get();


        //====== Modify/Update the setup file ======//
        CreateSetupFile();

        //====== Loop over flight conditions and solve ======//
        vector<double> alphaVec;
        vector<double> betaVec;
        vector<double> machVec;
        GetSweepVectors( alphaVec, betaVec, machVec );

        for ( int iAlpha = 0; iAlpha < alphaVec.size(); iAlpha++ )
        {
            //Set current alpha value
            double current_alpha = alphaVec[iAlpha];

            for ( int iBeta = 0; iBeta < betaVec.size(); iBeta++ )
            {
                //Set current beta value
                double current_beta = betaVec[iBeta];

                for ( int iMach = 0; iMach < machVec.size(); iMach++ )
                {
                    //Set current mach value
                    double current_mach = machVec[iMach];

                    //====== Clear VSPAERO output files ======//
                    if ( FileExist( adbFileName ) )
                    {
                        remove( adbFileName.c_str() );
                    }
                    if ( FileExist( historyFileName ) )
                    {
                        remove( historyFileName.c_str() );
                    }
                    if ( FileExist( loadFileName ) )
                    {
                        remove( loadFileName.c_str() );
                    }
                    if ( FileExist( stabFileName ) )
                    {
                        remove( stabFileName.c_str() );
                    }

                    //====== Send command to be executed by the system at the command prompt ======//
                    vector<string> args;
                    // Set mach, alpha, beta (save to local "current*" variables to use as header information in the results manager)
                    args.push_back( "-fs" );       // "freestream" override flag
                    args.push_back( StringUtil::double_to_string( current_mach, "%.2f" ) );
                    args.push_back( "END" );
                    args.push_back( StringUtil::double_to_string( current_alpha, "%.3f" ) );
                    args.push_back( "END" );
                    args.push_back( StringUtil::double_to_string( current_beta, "%.3f" ) );
                    args.push_back( "END" );
                    // Set number of openmp threads
                    args.push_back( "-omp" );
                    args.push_back( StringUtil::int_to_string( m_NCPU.Get(), "%d" ) );
                    // Set stability run arguments
                    if ( stabilityFlag )
                    {
                        switch ( stabilityType )
                        {
                            case vsp::STABILITY_DEFAULT:
                                args.push_back( "-stab" );
                                break;

                            case vsp::STABILITY_P_ANALYSIS:
                                args.push_back( "-pstab" );
                                break;

                            case vsp::STABILITY_Q_ANALYSIS:
                                args.push_back( "-qstab" );
                                break;

                            case vsp::STABILITY_R_ANALYSIS:
                                args.push_back( "-rstab" );
                                break;

                            // === To Be Implemented ===
                            //case vsp::STABILITY_HEAVE:
                            //    AnalysisType = "HEAVE";
                            //    break;

                            //case vsp::STABILITY_IMPULSE:
                            //    AnalysisType = "IMPULSE";
                            //    break;
                            
                            //case vsp::STABILITY_UNSTEADY:
                            //    args.push_back( "-unsteady" );
                            //    break;
                        }
                    }

                    if ( m_FromSteadyState() )
                    {
                        args.push_back( "-fromsteadystate" );
                    }

                    // Force averaging startign at wake iteration N
                    if( wakeAvgStartIter >= 1 )
                    {
                        args.push_back( "-avg" );
                        args.push_back( StringUtil::int_to_string( wakeAvgStartIter, "%d" ) );
                    }
                    if( wakeSkipUntilIter >= 1 )
                    {
                        // No wake for first N iterations
                        args.push_back( "-nowake" );
                        args.push_back( StringUtil::int_to_string( wakeSkipUntilIter, "%d" ) );
                    }

                    if ( m_GroundEffectToggle() )
                    {
                        args.push_back( "-groundheight" );
                        args.push_back( StringUtil::double_to_string( m_GroundEffect(), "%f" ) );
                    }

                    if( m_Write2DFEMFlag() )
                    {
                        args.push_back( "-write2dfem" );
                    }

                    if ( m_Precondition() == vsp::PRECON_JACOBI )
                    {
                        args.push_back( "-jacobi" );
                    }
                    else if ( m_Precondition() == vsp::PRECON_SSOR )
                    {
                        args.push_back( "-ssor" );
                    }

                    if ( m_KTCorrection() )
                    {
                        args.push_back( "-dokt" );
                    }

                    // Add model file name
                    args.push_back( modelNameBase );

                    //Print out execute command
                    string cmdStr = m_SolverProcess.PrettyCmd( veh->GetExePath(), veh->GetVSPAEROCmd(), args );
                    if( logFile )
                    {
                        fprintf( logFile, "%s", cmdStr.c_str() );
                    }
                    else
                    {
                        MessageData data;
                        data.m_String = "VSPAEROSolverMessage";
                        data.m_StringVec.push_back( cmdStr );
                        MessageMgr::getInstance().Send( "ScreenMgr", NULL, data );
                    }

                    // Execute VSPAero
                    m_SolverProcess.ForkCmd( veh->GetExePath(), veh->GetVSPAEROCmd(), args );

                    // ==== MonitorSolverProcess ==== //
                    MonitorSolver( logFile );


                    // Check if the kill solver flag has been raised, if so clean up and return
                    //  note: we could have exited the IsRunning loop if the process was killed
                    if( m_SolverProcessKill )
                    {
                        m_SolverProcessKill = false;    //reset kill flag

                        return string();    //return empty result ID vector
                    }

                    //====== Read in all of the results ======//
                    // read the files if there is new data that has not successfully been read in yet
                    ReadHistoryFile( historyFileName, res_id_vector, analysisMethod );
                    ReadLoadFile( loadFileName, res_id_vector, analysisMethod );
                    if ( stabilityFlag )
                    {
                        ReadStabFile( stabFileName, res_id_vector, analysisMethod, stabilityType );      //*.STAB stability coeff file
                    }

                    // CpSlice Latest *.adb File if slices are defined
                    if ( m_CpSliceFlag() && m_CpSliceVec.size() > 0 )
                    {
                        ComputeCpSlices();
                    }

                    // Send the message to update the screens
                    MessageData data;
                    data.m_String = "UpdateAllScreens";
                    MessageMgr::getInstance().Send( "ScreenMgr", NULL, data );

                }    //Mach sweep loop

            }    //beta sweep loop

        }    //alpha sweep loop

    }

    // Create "wrapper" result to contain a vector of result IDs (this maintains compatibility to return a single result after computation)
    Results *res = ResultsMgr.CreateResults( "VSPAERO_Wrapper" );
    if( !res )
    {
        return string();
    }
    else
    {
        res->Add( NameValData( "ResultsVec", res_id_vector ) );
        return res->GetID();
    }
}

/* ComputeSolverBatch(FILE * logFile)
*/
string VSPAEROMgrSingleton::ComputeSolverBatch( FILE * logFile )
{
    std::vector <string> res_id_vector;

    Vehicle *veh = VehicleMgr.GetVehicle();

    if ( veh )
    {

        string adbFileName = m_AdbFile;
        string historyFileName = m_HistoryFile;
        string loadFileName = m_LoadFile;
        string stabFileName = m_StabFile;
        string modelNameBase = m_ModelNameBase;

        bool stabilityFlag = m_StabilityCalcFlag.Get();
        vsp::VSPAERO_ANALYSIS_METHOD analysisMethod = ( vsp::VSPAERO_ANALYSIS_METHOD )m_AnalysisMethod.Get();
        vsp::VSPAERO_STABILITY_TYPE stabilityType = ( vsp::VSPAERO_STABILITY_TYPE )m_StabilityType.Get();

        int ncpu = m_NCPU.Get();

        int wakeAvgStartIter = m_WakeAvgStartIter.Get();
        int wakeSkipUntilIter = m_WakeSkipUntilIter.Get();


        //====== Modify/Update the setup file ======//
        if ( m_Verbose ) { printf( "Writing vspaero setup file: %s\n", m_SetupFile.c_str() ); }
        // if the setup file doesn't exist, create one with the current settings
        // TODO output a warning to the user that we are creating a default file
        CreateSetupFile();

        vector<double> alphaVec;
        vector<double> betaVec;
        vector<double> machVec;
        GetSweepVectors( alphaVec, betaVec, machVec );

        //====== Clear VSPAERO output files ======//
        if ( FileExist( adbFileName ) )
        {
            remove( adbFileName.c_str() );
        }
        if ( FileExist( historyFileName ) )
        {
            remove( historyFileName.c_str() );
        }
        if ( FileExist( loadFileName ) )
        {
            remove( loadFileName.c_str() );
        }
        if ( FileExist( stabFileName ) )
        {
            remove( stabFileName.c_str() );
        }

        //====== generate batch mode command to be executed by the system at the command prompt ======//
        vector<string> args;
        // Set mach, alpha, beta (save to local "current*" variables to use as header information in the results manager)
        args.push_back( "-fs" );       // "freestream" override flag

        //====== Loop over flight conditions and solve ======//
        // Mach
        for ( int iMach = 0; iMach < machVec.size(); iMach++ )
        {
            args.push_back( StringUtil::double_to_string( machVec[iMach], "%.2f" ) );
        }
        args.push_back( "END" );
        // Alpha
        for ( int iAlpha = 0; iAlpha < alphaVec.size(); iAlpha++ )
        {
            args.push_back( StringUtil::double_to_string( alphaVec[iAlpha], "%.3f" ) );
        }
        args.push_back( "END" );
        // Beta
        for ( int iBeta = 0; iBeta < betaVec.size(); iBeta++ )
        {
            args.push_back( StringUtil::double_to_string( betaVec[iBeta], "%.3f" ) );
        }
        args.push_back( "END" );

        // Set number of openmp threads
        args.push_back( "-omp" );
        args.push_back( StringUtil::int_to_string( m_NCPU.Get(), "%d" ) );

        // Set stability run arguments
        if ( stabilityFlag )
        {
            switch ( stabilityType )
            {
                case vsp::STABILITY_DEFAULT:
                    args.push_back( "-stab" );
                    break;

                case vsp::STABILITY_P_ANALYSIS:
                    args.push_back( "-pstab" );
                    break;

                case vsp::STABILITY_Q_ANALYSIS:
                    args.push_back( "-qstab" );
                    break;

                case vsp::STABILITY_R_ANALYSIS:
                    args.push_back( "-rstab" );
                    break;

                // === To Be Implemented ===
                //case vsp::STABILITY_HEAVE:
                //    AnalysisType = "HEAVE";
                //    break;

                //case vsp::STABILITY_IMPULSE:
                //    AnalysisType = "IMPULSE";
                //    break;

                //case vsp::STABILITY_UNSTEADY:
                //    args.push_back( "-unsteady" );
                //    break;
            }
        }

        if ( m_FromSteadyState() )
        {
            args.push_back( "-fromsteadystate" );
        }

        // Force averaging startign at wake iteration N
        if( wakeAvgStartIter >= 1 )
        {
            args.push_back( "-avg" );
            args.push_back( StringUtil::int_to_string( wakeAvgStartIter, "%d" ) );
        }
        if( wakeSkipUntilIter >= 1 )
        {
            // No wake for first N iterations
            args.push_back( "-nowake" );
            args.push_back( StringUtil::int_to_string( wakeSkipUntilIter, "%d" ) );
        }

        if ( m_GroundEffectToggle() )
        {
            args.push_back( "-groundheight" );
            args.push_back( StringUtil::double_to_string( m_GroundEffect(), "%f" ) );
        }

        if( m_Write2DFEMFlag() )
        {
            args.push_back( "-write2dfem" );
        }

        if ( m_Precondition() == vsp::PRECON_JACOBI )
        {
            args.push_back( "-jacobi" );
        }
        else if ( m_Precondition() == vsp::PRECON_SSOR )
        {
            args.push_back( "-ssor" );
        }

        if ( m_KTCorrection() )
        {
            args.push_back( "-dokt" );
        }

        // Add model file name
        args.push_back( modelNameBase );

        //Print out execute command
        string cmdStr = m_SolverProcess.PrettyCmd( veh->GetExePath(), veh->GetVSPAEROCmd(), args );
        if( logFile )
        {
            fprintf( logFile, "%s", cmdStr.c_str() );
        }
        else
        {
            MessageData data;
            data.m_String = "VSPAEROSolverMessage";
            data.m_StringVec.push_back( cmdStr );
            MessageMgr::getInstance().Send( "ScreenMgr", NULL, data );
        }

        // Execute VSPAero
        m_SolverProcess.ForkCmd( veh->GetExePath(), veh->GetVSPAEROCmd(), args );

        // ==== MonitorSolverProcess ==== //
        MonitorSolver( logFile );


        // Check if the kill solver flag has been raised, if so clean up and return
        //  note: we could have exited the IsRunning loop if the process was killed
        if( m_SolverProcessKill )
        {
            m_SolverProcessKill = false;    //reset kill flag

            return string();    //return empty result ID vector
        }

        //====== Read in all of the results ======//
        ReadHistoryFile( historyFileName, res_id_vector, analysisMethod );
        ReadLoadFile( loadFileName, res_id_vector, analysisMethod );
        if ( stabilityFlag )
        {
            ReadStabFile( stabFileName, res_id_vector, analysisMethod, stabilityType );      //*.STAB stability coeff file
        }

        // CpSlice *.adb File and slices are defined
        if ( m_CpSliceFlag() && m_CpSliceVec.size() > 0 )
        {
            ComputeCpSlices();
        }

        // Send the message to update the screens
        MessageData data;
        data.m_String = "UpdateAllScreens";
        MessageMgr::getInstance().Send( "ScreenMgr", NULL, data );

    }

    // Create "wrapper" result to contain a vector of result IDs (this maintains compatibility to return a single result after computation)
    Results *res = ResultsMgr.CreateResults( "VSPAERO_Wrapper" );
    if( !res )
    {
        return string();
    }
    else
    {
        res->Add( NameValData( "ResultsVec", res_id_vector ) );
        return res->GetID();
    }
}

void VSPAEROMgrSingleton::MonitorSolver( FILE * logFile )
{
    // ==== MonitorSolverProcess ==== //
    int bufsize = 1000;
    char *buf;
    buf = ( char* ) malloc( sizeof( char ) * ( bufsize + 1 ) );
    unsigned long nread = 1;
    bool runflag = m_SolverProcess.IsRunning();
    while ( runflag || ( nread > 0 && nread != ( unsigned long ) - 1 ) )
    {
        m_SolverProcess.ReadStdoutPipe( buf, bufsize, &nread );
        if( nread > 0 && nread != ( unsigned long ) - 1 )
        {
            if ( buf )
            {
                buf[nread] = 0;
                StringUtil::change_from_to( buf, '\r', '\n' );
                if( logFile )
                {
                    fprintf( logFile, "%s", buf );
                }
                else
                {
                    MessageData data;
                    data.m_String = "VSPAEROSolverMessage";
                    data.m_StringVec.push_back( string( buf ) );
                    MessageMgr::getInstance().Send( "ScreenMgr", NULL, data );
                }
            }
        }

        SleepForMilliseconds( 100 );
        runflag = m_SolverProcess.IsRunning();
    }

#ifdef WIN32
    CloseHandle( m_SolverProcess.m_StdoutPipe[0] );
    m_SolverProcess.m_StdoutPipe[0] = NULL;
#else
    close( m_SolverProcess.m_StdoutPipe[0] );
    m_SolverProcess.m_StdoutPipe[0] = -1;
#endif

    free( buf );
}

void VSPAEROMgrSingleton::AddResultHeader( string res_id, double mach, double alpha, double beta, vsp::VSPAERO_ANALYSIS_METHOD analysisMethod )
{
    // Add Flow Condition header to each result
    Results * res = ResultsMgr.FindResultsPtr( res_id );
    if ( res )
    {
        res->Add( NameValData( "AnalysisMethod", analysisMethod ) );
    }
}

// helper thread functions for VSPAERO GUI interface and multi-threaded impleentation
bool VSPAEROMgrSingleton::IsSolverRunning()
{
    return m_SolverProcess.IsRunning();
}

void VSPAEROMgrSingleton::KillSolver()
{
    // Raise flag to break the compute solver thread
    m_SolverProcessKill = true;
    return m_SolverProcess.Kill();
}

ProcessUtil* VSPAEROMgrSingleton::GetSolverProcess()
{
    return &m_SolverProcess;
}

// function is used to wait for the result to show up on the file system
int VSPAEROMgrSingleton::WaitForFile( string filename )
{
    // Wait until the results show up on the file system
    int n_wait = 0;
    // wait no more than 5 seconds = (50*100)/1000
    while ( ( !FileExist( filename ) ) & ( n_wait < 50 ) )
    {
        n_wait++;
        SleepForMilliseconds( 100 );
    }
    SleepForMilliseconds( 100 );  //additional wait for file

    if ( FileExist( filename ) )
    {
        return vsp::VSP_OK;
    }
    else
    {
        return vsp::VSP_FILE_DOES_NOT_EXIST;
    }
}

/*******************************************************
Read .HISTORY file output from VSPAERO
analysisMethod is passed in because the parm it is set by might change by the time we are done calculating the solution
See: VSP_Solver.C in vspaero project
line 4351 - void VSP_SOLVER::OutputStatusFile(int Type)
line 4407 - void VSP_SOLVER::OutputZeroLiftDragToStatusFile(void)
TODO:
- Update this function to use the generic table read as used in: string VSPAEROMgrSingleton::ReadStabFile()
*******************************************************/
void VSPAEROMgrSingleton::ReadHistoryFile( string filename, vector <string> &res_id_vector, vsp::VSPAERO_ANALYSIS_METHOD analysisMethod )
{
    //TODO return success or failure
    FILE *fp = NULL;
    //size_t result;
    bool read_success = false;

    //HISTORY file
    WaitForFile( filename );
    fp = fopen( filename.c_str(), "r" );
    if ( fp == NULL )
    {
        fprintf( stderr, "ERROR %d: Could not open History file: %s\n\tFile: %s \tLine:%d\n", vsp::VSP_FILE_DOES_NOT_EXIST, m_HistoryFile.c_str(), __FILE__, __LINE__ );
        return;
    }

    Results* res = NULL;
    std::vector<string> data_string_array;

    char seps[]   = " :,\t\n";
    while ( !feof( fp ) )
    {
        data_string_array = ReadDelimLine( fp, seps ); //this is also done in some of the embedded loops below

        if ( CheckForCaseHeader( data_string_array ) )
        {
            res = ResultsMgr.CreateResults( "VSPAERO_History" );
            res_id_vector.push_back( res->GetID() );

            if ( ReadVSPAEROCaseHeader( res, fp, analysisMethod ) != 0 )
            {
                // Failed to read the case header
                fprintf( stderr, "ERROR %d: Could not read case header in VSPAERO file: %s\n\tFile: %s \tLine:%d\n", vsp::VSP_FILE_READ_FAILURE, m_StabFile.c_str(), __FILE__, __LINE__ );
                return;
            }

        }

        //READ wake iteration table
        /* Example wake iteration table
        Iter      Mach       AoA      Beta       CL         CDo       CDi      CDtot      CS        L/D        E        CFx       CFy       CFz       CMx       CMy       CMz       T/QS
        1   0.00000   1.00000   0.00000   0.03329   0.00364   0.00009   0.00373  -0.00000   8.93773 395.42033  -0.00049  -0.00000   0.03329  -0.00000  -0.09836  -0.00000   0.00000
        2   0.00000   1.00000   0.00000   0.03329   0.00364   0.00009   0.00373  -0.00000   8.93494 394.87228  -0.00049  -0.00000   0.03328  -0.00000  -0.09834  -0.00000   0.00000
        ...
        */
        int wake_iter_table_columns = 18;

        bool unsteady = false;
        int unsteady_table_columns = 28;
        if ( data_string_array.size() == unsteady_table_columns )
        {
            unsteady = true;
        }

        if( data_string_array.size() >= wake_iter_table_columns )
        {
            //discard the header row and read the next line assuming that it is numeric
            data_string_array = ReadDelimLine( fp, seps );

            // create new vectors for this set of results information
            std::vector<int> i;
            std::vector<double> time;
            std::vector<double> Mach;
            std::vector<double> Alpha;
            std::vector<double> Beta;
            std::vector<double> CL;
            std::vector<double> CDo;
            std::vector<double> CDi;
            std::vector<double> CDtot;
            std::vector<double> CS;
            std::vector<double> LoD;
            std::vector<double> E;
            std::vector<double> CFx;
            std::vector<double> CFy;
            std::vector<double> CFz;
            std::vector<double> CMx;
            std::vector<double> CMy;
            std::vector<double> CMz;
            std::vector<double> ToQS;
            std::vector<double> UnstdAng;
            std::vector<double> CL_Un;
            std::vector<double> CDi_Un;
            std::vector<double> CS_Un;
            std::vector<double> CFx_Un;
            std::vector<double> CFy_Un;
            std::vector<double> CFz_Un;
            std::vector<double> CMx_Un;
            std::vector<double> CMy_Un;
            std::vector<double> CMz_Un;

            while ( data_string_array.size() >= wake_iter_table_columns )
            {
                if ( unsteady )
                {
                    time.push_back( std::stod( data_string_array[0] ) );
                }
                else
                {
                    i.push_back( std::stoi( data_string_array[0] ) );
                }

                Mach.push_back(     std::stod( data_string_array[1] ) );
                Alpha.push_back(    std::stod( data_string_array[2] ) );
                Beta.push_back(     std::stod( data_string_array[3] ) );

                CL.push_back(       std::stod( data_string_array[4] ) );
                CDo.push_back(      std::stod( data_string_array[5] ) );
                CDi.push_back(      std::stod( data_string_array[6] ) );
                CDtot.push_back(    std::stod( data_string_array[7] ) );
                CS.push_back(       std::stod( data_string_array[8] ) );

                LoD.push_back(      std::stod( data_string_array[9] ) );
                E.push_back(        std::stod( data_string_array[10] ) );

                CFx.push_back(      std::stod( data_string_array[11] ) );
                CFy.push_back(      std::stod( data_string_array[12] ) );
                CFz.push_back(      std::stod( data_string_array[13] ) );

                CMx.push_back(      std::stod( data_string_array[14] ) );
                CMy.push_back(      std::stod( data_string_array[15] ) );
                CMz.push_back(      std::stod( data_string_array[16] ) );

                ToQS.push_back(     std::stod( data_string_array[17] ) );

                if ( unsteady ) // Additional columns for unsteady analysis
                {
                    UnstdAng.push_back( std::stod( data_string_array[18] ) );
                    CL_Un.push_back( std::stod( data_string_array[19] ) );
                    CDi_Un.push_back( std::stod( data_string_array[20] ) );
                    CS_Un.push_back( std::stod( data_string_array[21] ) );
                    CFx_Un.push_back( std::stod( data_string_array[22] ) );
                    CFy_Un.push_back( std::stod( data_string_array[23] ) );
                    CFz_Un.push_back( std::stod( data_string_array[24] ) );
                    CMx_Un.push_back( std::stod( data_string_array[25] ) );
                    CMy_Un.push_back( std::stod( data_string_array[26] ) );
                    CMz_Un.push_back( std::stod( data_string_array[27] ) );
                }

                data_string_array = ReadDelimLine( fp, seps );
            }

            //add to the results manager
            if ( res )
            {
                if ( unsteady )
                {
                    res->Add( NameValData( "Time", time ) );
                }
                else
                {
                    res->Add( NameValData( "WakeIter", i ) );
                }
                res->Add( NameValData( "Mach", Mach ) );
                res->Add( NameValData( "Alpha", Alpha ) );
                res->Add( NameValData( "Beta", Beta ) );
                res->Add( NameValData( "CL", CL ) );
                res->Add( NameValData( "CDo", CDo ) );
                res->Add( NameValData( "CDi", CDi ) );
                res->Add( NameValData( "CDtot", CDtot ) );
                res->Add( NameValData( "CS", CS ) );
                res->Add( NameValData( "L/D", LoD ) );
                res->Add( NameValData( "E", E ) );
                res->Add( NameValData( "CFx", CFx ) );
                res->Add( NameValData( "CFy", CFy ) );
                res->Add( NameValData( "CFz", CFz ) );
                res->Add( NameValData( "CMx", CMx ) );
                res->Add( NameValData( "CMy", CMy ) );
                res->Add( NameValData( "CMz", CMz ) );
                res->Add( NameValData( "T/QS", ToQS ) );

                if ( unsteady )
                {
                    res->Add( NameValData( "UnstdyAng", UnstdAng ) );
                    res->Add( NameValData( "CL_Un", CL_Un ) );
                    res->Add( NameValData( "CDi_Un", CDi_Un ) );
                    res->Add( NameValData( "CS_Un", CS_Un ) );
                    res->Add( NameValData( "CFx_Un", CFx_Un ) );
                    res->Add( NameValData( "CFy_Un", CFy_Un ) );
                    res->Add( NameValData( "CFz_Un", CFz_Un ) );
                    res->Add( NameValData( "CMx_Un", CMx_Un ) );
                    res->Add( NameValData( "CMy_Un", CMy_Un ) );
                    res->Add( NameValData( "CMz_Un", CMz_Un ) );
                }
            }

        } // end of wake iteration

    } //end feof loop to read entire history file

    fclose ( fp );

    return;
}

/*******************************************************
Read .LOD file output from VSPAERO
See: VSP_Solver.C in vspaero project
line 2851 - void VSP_SOLVER::CalculateSpanWiseLoading(void)
TODO:
- Update this function to use the generic table read as used in: string VSPAEROMgrSingleton::ReadStabFile()
- Read in Component table information, this is the 2nd table at the bottom of the .lod file
*******************************************************/
void VSPAEROMgrSingleton::ReadLoadFile( string filename, vector <string> &res_id_vector, vsp::VSPAERO_ANALYSIS_METHOD analysisMethod )
{
    FILE *fp = NULL;
    bool read_success = false;

    //LOAD file
    WaitForFile( filename );
    fp = fopen( filename.c_str(), "r" );
    if ( fp == NULL )
    {
        fprintf( stderr, "ERROR %d: Could not open Load file: %s\n\tFile: %s \tLine:%d\n", vsp::VSP_FILE_DOES_NOT_EXIST, m_LoadFile.c_str(), __FILE__, __LINE__ );
        return;
    }

    Results* res = NULL;
    std::vector< std::string > data_string_array;
    std::vector< std::vector< double > > data_array;

    double cref = 1.0;

    char seps[]   = " :,\t\n";
    while ( !feof( fp ) )
    {
        data_string_array = ReadDelimLine( fp, seps ); //this is also done in some of the embedded loops below

        if ( CheckForCaseHeader( data_string_array ) )
        {
            res = ResultsMgr.CreateResults( "VSPAERO_Load" );
            res_id_vector.push_back( res->GetID() );

            if ( ReadVSPAEROCaseHeader( res, fp, analysisMethod ) != 0 )
            {
                // Failed to read the case header
                fprintf( stderr, "ERROR %d: Could not read case header in VSPAERO file: %s\n\tFile: %s \tLine:%d\n", vsp::VSP_FILE_READ_FAILURE, m_StabFile.c_str(), __FILE__, __LINE__ );
                return;
            }

            cref = res->FindPtr( "FC_Cref_" )->GetDouble( 0 );

        }

        // Sectional distribution table
        int nSectionalDataTableCols = 14;
        if ( data_string_array.size() == nSectionalDataTableCols && data_string_array[0].find( "Comp" ) == std::string::npos && !isdigit( data_string_array[0][0] ) ) // TODO: Test more thoroughly
        {
            //discard the header row and read the next line assuming that it is numeric
            data_string_array = ReadDelimLine( fp, seps );

            // Raw data vectors
            std::vector<int> WingId;
            std::vector<double> S;
            std::vector<double> Yavg;
            std::vector<double> Chord;
            std::vector<double> VoVref;
            std::vector<double> Cl;
            std::vector<double> Cd;
            std::vector<double> Cs;
            std::vector<double> Cx;
            std::vector<double> Cy;
            std::vector<double> Cz;
            std::vector<double> Cmx;
            std::vector<double> Cmy;
            std::vector<double> Cmz;

            //normalized by local chord
            std::vector<double> Clc_cref;
            std::vector<double> Cdc_cref;
            std::vector<double> Csc_cref;
            std::vector<double> Cxc_cref;
            std::vector<double> Cyc_cref;
            std::vector<double> Czc_cref;
            std::vector<double> Cmxc_cref;
            std::vector<double> Cmyc_cref;
            std::vector<double> Cmzc_cref;

            double chordRatio;

            // read the data rows
            while ( data_string_array.size() == nSectionalDataTableCols && data_string_array[0].find( "Comp" ) == std::string::npos )
            {
                // Store the raw data
                WingId.push_back( std::stoi( data_string_array[0] ) );
                S.push_back( std::stod( data_string_array[1] ) );
                Yavg.push_back(   std::stod( data_string_array[2] ) );
                Chord.push_back(  std::stod( data_string_array[3] ) );
                VoVref.push_back( std::stod( data_string_array[4] ) );
                Cl.push_back(     std::stod( data_string_array[5] ) );
                Cd.push_back(     std::stod( data_string_array[6] ) );
                Cs.push_back(     std::stod( data_string_array[7] ) );
                Cx.push_back(     std::stod( data_string_array[8] ) );
                Cy.push_back(     std::stod( data_string_array[9] ) );
                Cz.push_back(     std::stod( data_string_array[10] ) );
                Cmx.push_back(    std::stod( data_string_array[11] ) );
                Cmy.push_back(    std::stod( data_string_array[12] ) );
                Cmz.push_back(    std::stod( data_string_array[13] ) );

                chordRatio = Chord.back() / cref;

                // Normalized by local chord
                Clc_cref.push_back( Cl.back() * chordRatio );
                Cdc_cref.push_back( Cd.back() * chordRatio );
                Csc_cref.push_back( Cs.back() * chordRatio );
                Cxc_cref.push_back( Cx.back() * chordRatio );
                Cyc_cref.push_back( Cy.back() * chordRatio );
                Czc_cref.push_back( Cz.back() * chordRatio );
                Cmxc_cref.push_back( Cmx.back() * chordRatio );
                Cmyc_cref.push_back( Cmy.back() * chordRatio );
                Cmzc_cref.push_back( Cmz.back() * chordRatio );

                // Read the next line and loop
                data_string_array = ReadDelimLine( fp, seps );
            }

            // Finish up by adding the data to the result res
            res->Add( NameValData( "WingId", WingId ) );
            res->Add( NameValData( "S", S ) );
            res->Add( NameValData( "Yavg", Yavg ) );
            res->Add( NameValData( "Chord", Chord ) );
            res->Add( NameValData( "V/Vref", VoVref ) );
            res->Add( NameValData( "cl", Cl ) );
            res->Add( NameValData( "cd", Cd ) );
            res->Add( NameValData( "cs", Cs ) );
            res->Add( NameValData( "cx", Cx ) );
            res->Add( NameValData( "cy", Cy ) );
            res->Add( NameValData( "cz", Cz ) );
            res->Add( NameValData( "cmx", Cmx ) );
            res->Add( NameValData( "cmy", Cmy ) );
            res->Add( NameValData( "cmz", Cmz ) );

            res->Add( NameValData( "cl*c/cref", Clc_cref ) );
            res->Add( NameValData( "cd*c/cref", Cdc_cref ) );
            res->Add( NameValData( "cs*c/cref", Csc_cref ) );
            res->Add( NameValData( "cx*c/cref", Cxc_cref ) );
            res->Add( NameValData( "cy*c/cref", Cyc_cref ) );
            res->Add( NameValData( "cz*c/cref", Czc_cref ) );
            res->Add( NameValData( "cmx*c/cref", Cmxc_cref ) );
            res->Add( NameValData( "cmy*c/cref", Cmyc_cref ) );
            res->Add( NameValData( "cmz*c/cref", Cmzc_cref ) );

        } // end sectional table read

    } // end file loop

    std::fclose ( fp );

    return;
}

/*******************************************************
Read .STAB file output from VSPAERO
See: VSP_Solver.C in vspaero project
*******************************************************/
void VSPAEROMgrSingleton::ReadStabFile( string filename, vector <string> &res_id_vector, vsp::VSPAERO_ANALYSIS_METHOD analysisMethod, vsp::VSPAERO_STABILITY_TYPE stabilityType )
{
    FILE *fp = NULL;
    bool read_success = false;
    WaitForFile( filename );
    fp = fopen( filename.c_str() , "r" );
    if ( fp == NULL )
    {
        fprintf( stderr, "ERROR %d: Could not open Stab file: %s\n\tFile: %s \tLine:%d\n", vsp::VSP_FILE_DOES_NOT_EXIST, m_StabFile.c_str(), __FILE__, __LINE__ );
        return;
    }

    Results* res = NULL;

    std::vector<string> table_column_names;
    std::vector<string> data_string_array;

    // Read in all of the data into the results manager
    char seps[] = " :,\t\n";
    while ( !feof( fp ) )
    {
        data_string_array = ReadDelimLine( fp, seps ); //this is also done in some of the embedded loops below

        if ( CheckForCaseHeader( data_string_array ) )
        {
            res = ResultsMgr.CreateResults( "VSPAERO_Stab" );
            res->Add( NameValData( "StabilityType", stabilityType ) );
            res_id_vector.push_back( res->GetID() );

            if ( ReadVSPAEROCaseHeader( res, fp, analysisMethod ) != 0 )
            {
                // Failed to read the case header
                fprintf( stderr, "ERROR %d: Could not read case header in VSPAERO file: %s\n\tFile: %s \tLine:%d\n", vsp::VSP_FILE_READ_FAILURE, m_StabFile.c_str(), __FILE__, __LINE__ );
                return;
            }
        }
        else if ( res && CheckForResultHeader( data_string_array ) )
        {
            data_string_array = ReadDelimLine( fp, seps );

            // Read result table
            double value;

            // Parse if this is not a comment line
            while ( !feof( fp ) && strncmp( data_string_array[0].c_str(), "#", 1 ) != 0 )
            {
                if ( ( data_string_array.size() == 3 ) )
                {
                    // assumption that the 2nd entry is a number
                    if ( sscanf( data_string_array[1].c_str(), "%lf", &value ) == 1 )
                    {
                        res->Add( NameValData( data_string_array[0], value ) );
                    }
                }

                // read the next line
                data_string_array = ReadDelimLine( fp, seps );
            } // end while
        }
        else if ( data_string_array.size() > 0 )
        {
            // Parse if this is not a comment line
            if ( res && strncmp( data_string_array[0].c_str(), "#", 1 ) != 0 )
            {
                if ( stabilityType != vsp::STABILITY_DEFAULT )
                {
                    // Support file format for P, Q, or R uinsteady analysis types
                    string name = data_string_array[0];

                    for ( unsigned int i_field = 1; i_field < data_string_array.size(); i_field++ )
                    {
                        //attempt to read a double if that fails then treat it as a string result and add to result name to account for spaces
                        double temp_val = 0;
                        int result = 0;
                        result = sscanf( data_string_array[i_field].c_str(), "%lf", &temp_val );

                        if ( result == 1 )
                        {
                            res->Add( NameValData( name, temp_val ) );
                        }
                        else
                        {
                            name += data_string_array[i_field];
                        }
                    }
                }
                else
                {
                    //================ Table Data ================//
                    // Checks for table header format
                    if ( ( data_string_array.size() != table_column_names.size() ) || ( table_column_names.size() == 0 ) )
                    {
                        //Indicator that the data table has changed or has not been initialized.
                        table_column_names.clear();
                        table_column_names = data_string_array;

                        // map control group names to full control surface group names
                        int i_field_offset = -1;
                        for ( unsigned int i_field = 0; i_field < data_string_array.size(); i_field++ )
                        {
                            if ( strstr( table_column_names[i_field].c_str(), "ConGrp_" ) )
                            {
                                //  Set field offset based on the first ConGrp_ found
                                if ( i_field_offset == -1 )
                                {
                                    i_field_offset = i_field;
                                }

                                if ( m_Verbose )
                                {
                                    printf( "\tMapping table col name to CSG name: \n" );
                                }
                                if ( m_Verbose )
                                {
                                    printf( "\ti_field = %d --> i_field_offset = %d\n", i_field, i_field - i_field_offset );
                                }
                                if ( ( i_field - i_field_offset ) < m_ControlSurfaceGroupVec.size() )
                                {
                                    if ( m_Verbose )
                                    {
                                        printf( "\t%s --> %s\n", table_column_names[i_field].c_str(), m_ControlSurfaceGroupVec[i_field - i_field_offset]->GetName().c_str() );
                                    }
                                    table_column_names[i_field] = m_ControlSurfaceGroupVec[i_field - i_field_offset]->GetName();
                                }
                                else
                                {
                                    printf( "\tERROR (i_field - i_field_offset) > m_ControlSurfaceGroupVec.size()\n" );
                                    printf( "\t      (  %d    -    %d         ) >            %lu             \n", i_field, i_field_offset, m_ControlSurfaceGroupVec.size() );
                                }

                            }
                        }

                    }
                    else
                    {
                        //This is a continuation of the current table and add this row to the results manager
                        for ( unsigned int i_field = 1; i_field < data_string_array.size(); i_field++ )
                        {
                            //attempt to read a double if that fails then treat it as a string result
                            double temp_val = 0;
                            int result = 0;
                            result = sscanf( data_string_array[i_field].c_str(), "%lf", &temp_val );
                            if ( result == 1 )
                            {
                                res->Add( NameValData( data_string_array[0] + "_" + table_column_names[i_field], temp_val ) );
                            }
                            else
                            {
                                res->Add( NameValData( data_string_array[0] + "_" + table_column_names[i_field], data_string_array[i_field] ) );
                            }
                        }
                    } //end new table check

                } // end unsteady check

            } // end comment line check

        } // end data_string_array.size()>0 check

    } //end for while !feof(fp)

    std::fclose ( fp );

    return;
}

vector <string> VSPAEROMgrSingleton::ReadDelimLine( FILE * fp, char * delimeters )
{

    vector <string> dataStringVector;
    dataStringVector.clear();

    char strbuff[1024];                // buffer for entire line in file
    if ( fgets( strbuff, 1024, fp ) != NULL )
    {
        char * pch = strtok ( strbuff, delimeters );
        while ( pch != NULL )
        {
            dataStringVector.push_back( pch );
            pch = strtok ( NULL, delimeters );
        }
    }

    return dataStringVector;
}

bool VSPAEROMgrSingleton::CheckForCaseHeader( std::vector<string> headerStr )
{
    if ( headerStr.size() == 1 )
    {
        if ( strcmp( headerStr[0].c_str(), "*****************************************************************************************************************************************************************************************" ) == 0 )
        {
            return true;
        }
    }

    return false;
}

bool VSPAEROMgrSingleton::CheckForResultHeader( std::vector<string> headerStr )
{
    if ( headerStr.size() == 4 )
    {
        if ( strcmp( headerStr[0].c_str(), "#" ) == 0 && strcmp( headerStr[1].c_str(), "Result" ) == 0 )
        {
            return true;
        }
    }

    return false;
}

int VSPAEROMgrSingleton::ReadVSPAEROCaseHeader( Results * res, FILE * fp, vsp::VSPAERO_ANALYSIS_METHOD analysisMethod )
{
    // check input arguments
    if ( res == NULL )
    {
        // Bad pointer
        fprintf( stderr, "ERROR %d: Invalid results pointer\n\tFile: %s \tLine:%d\n", vsp::VSP_INVALID_PTR, __FILE__, __LINE__ );
        return -1;
    }
    if ( fp == NULL )
    {
        // Bad pointer
        fprintf( stderr, "ERROR %d: Invalid file pointer\n\tFile: %s \tLine:%d\n", vsp::VSP_INVALID_PTR, __FILE__, __LINE__ );
        return -2;
    }

    char seps[]   = " :,\t\n";
    std::vector<string> data_string_array;

    //skip any blank lines before the header
    while ( !feof( fp ) && data_string_array.size() == 0 )
    {
        data_string_array = ReadDelimLine( fp, seps ); //this is also done in some of the embedded loops below
    }

    // Read header table
    bool needs_header = true;
    bool mach_found = false;
    bool alpha_found = false;
    bool beta_found = false;
    double current_mach = -FLT_MAX;
    double current_alpha = -FLT_MAX;
    double current_beta = -FLT_MAX;
    double value;
    while ( !feof( fp ) && data_string_array.size() > 0 )
    {
        // Parse if this is not a comment line
        if ( ( strncmp( data_string_array[0].c_str(), "#", 1 ) != 0 ) && ( data_string_array.size() == 3 ) )
        {
            // assumption that the 2nd entry is a number
            if ( sscanf( data_string_array[1].c_str(), "%lf", &value ) == 1 )
            {
                res->Add( NameValData( "FC_" + data_string_array[0], value ) );

                // save flow condition information to be added to the header later
                if ( strcmp( data_string_array[0].c_str(), "Mach_" ) == 0 )
                {
                    current_mach = value;
                    mach_found = true;
                }
                if ( strcmp( data_string_array[0].c_str(), "AoA_" ) == 0 )
                {
                    current_alpha = value;
                    alpha_found = true;
                }
                if ( strcmp( data_string_array[0].c_str(), "Beta_" ) == 0 )
                {
                    current_beta = value;
                    beta_found = true;
                }

                // check if the information needed for the result header has been read in
                if ( mach_found && alpha_found && beta_found && needs_header )
                {
                    AddResultHeader( res->GetID(), current_mach, current_alpha, current_beta, analysisMethod );
                    needs_header = false;
                }
            }
        }

        // read the next line
        data_string_array = ReadDelimLine( fp, seps );

    } // end while

    if ( needs_header )
    {
        fprintf( stderr, "WARNING: Case header incomplete \n\tFile: %s \tLine:%d\n", __FILE__, __LINE__ );
        return -3;
    }

    return 0; // no errors
}

//Export Results to CSV
int VSPAEROMgrSingleton::ExportResultsToCSV( string fileName )
{
    int retVal = vsp::VSP_FILE_WRITE_FAILURE;

    // Get the results
    string resId = ResultsMgr.FindLatestResultsID( "VSPAERO_Wrapper" );
    if ( resId == string() )
    {
        retVal = vsp::VSP_CANT_FIND_NAME;
        fprintf( stderr, "ERROR %d: Unable to find ""VSPAERO_Wrapper"" result \n\tFile: %s \tLine:%d\n", retVal, __FILE__, __LINE__ );
        return retVal;
    }

    Results* resptr = ResultsMgr.FindResultsPtr( resId );
    if ( !resptr )
    {
        retVal = vsp::VSP_INVALID_PTR;
        fprintf( stderr, "ERROR %d: Unable to get pointer to ""VSPAERO_Wrapper"" result \n\tFile: %s \tLine:%d\n", retVal, __FILE__, __LINE__ );
        return retVal;
    }

    // Get all the child results and write out to csv using a vector of results
    vector <string> resIdVector = ResultsMgr.GetStringResults( resId, "ResultsVec" );
    if ( resIdVector.size() == 0 )
    {
        fprintf( stderr, "WARNING %d: ""VSPAERO_Wrapper"" result contains no child results \n\tFile: %s \tLine:%d\n", retVal, __FILE__, __LINE__ );
    }

    // Export to CSV file
    retVal = ResultsMgr.WriteCSVFile( fileName, resIdVector );

    // Check that the file made it to the file system and return status
    return WaitForFile( fileName );
}

void VSPAEROMgrSingleton::AddRotorDisk()
{
    RotorDisk* new_rd = new RotorDisk;
    new_rd->SetParentContainer( GetID() );
    m_RotorDiskVec.push_back( new_rd );
}

bool VSPAEROMgrSingleton::ValidRotorDiskIndex( int index )
{
    if ( ( index >= 0 ) && ( index < m_RotorDiskVec.size() ) && m_RotorDiskVec.size() > 0 )
    {
        return true;
    }

    return false;
}

void VSPAEROMgrSingleton::UpdateRotorDiskSuffix()
{
    for (int i = 0 ; i < (int) m_RotorDiskVec.size(); ++i)
    {
        m_RotorDiskVec[i]->SetParentContainer( GetID() );
        m_RotorDiskVec[i]->SetGroupDisplaySuffix( i );
    }
}

void VSPAEROMgrSingleton::UpdateControlSurfaceGroupSuffix()
{
    for (int i = 0 ; i < (int) m_ControlSurfaceGroupVec.size(); ++i)
    {
        m_ControlSurfaceGroupVec[i]->SetParentContainer( GetID() );
        m_ControlSurfaceGroupVec[i]->SetGroupDisplaySuffix( i );
    }
}

void VSPAEROMgrSingleton::AddControlSurfaceGroup()
{
    ControlSurfaceGroup* new_cs = new ControlSurfaceGroup;
    new_cs->SetParentContainer( GetID() );
    m_ControlSurfaceGroupVec.push_back( new_cs );

    m_CurrentCSGroupIndex = m_ControlSurfaceGroupVec.size() - 1;

    m_SelectedGroupedCS.clear();
    UpdateActiveControlSurfVec();

    HighlightSelected( CONTROL_SURFACE );
}

void VSPAEROMgrSingleton::RemoveControlSurfaceGroup()
{
    if ( m_CurrentCSGroupIndex != -1 )
    {
        for ( size_t i = 0; i < m_ActiveControlSurfaceVec.size(); ++i )
        {
            for ( size_t j = 0; j < m_CompleteControlSurfaceVec.size(); ++j )
            {
                if ( m_CompleteControlSurfaceVec[j].SSID.compare( m_ActiveControlSurfaceVec[i].SSID ) == 0 )
                {
                    m_CompleteControlSurfaceVec[j].isGrouped = false;
                }
            }
        }

        delete m_ControlSurfaceGroupVec[m_CurrentCSGroupIndex];
        m_ControlSurfaceGroupVec.erase( m_ControlSurfaceGroupVec.begin() + m_CurrentCSGroupIndex );

        if ( m_ControlSurfaceGroupVec.size() > 0 )
        {
            m_CurrentCSGroupIndex = 0;
        }
        else
        {
            m_CurrentCSGroupIndex = -1;
        }
    }
    m_SelectedGroupedCS.clear();
    UpdateActiveControlSurfVec();
    UpdateControlSurfaceGroupSuffix();
}

void VSPAEROMgrSingleton::AddSelectedToCSGroup()
{
    vector < int > selected = m_SelectedUngroupedCS;
    if ( m_CurrentCSGroupIndex != -1 )
    {
        vector < VspAeroControlSurf > ungrouped_vec = GetAvailableCSVec();

        for ( size_t i = 0; i < selected.size(); ++i )
        {
            m_ControlSurfaceGroupVec[ m_CurrentCSGroupIndex ]->AddSubSurface( ungrouped_vec[ selected[ i ] - 1 ] );
        }
    }
    m_SelectedUngroupedCS.clear();
    m_SelectedGroupedCS.clear();
    UpdateActiveControlSurfVec();
}

void VSPAEROMgrSingleton::AddAllToCSGroup()
{
    if ( m_CurrentCSGroupIndex != -1 )
    {
        vector < VspAeroControlSurf > ungrouped_vec = GetAvailableCSVec();
        for ( size_t i = 0; i < ungrouped_vec.size(); ++i )
        {
            m_ControlSurfaceGroupVec[ m_CurrentCSGroupIndex ]->AddSubSurface( ungrouped_vec[ i ] );
        }
    }
    m_SelectedUngroupedCS.clear();
    m_SelectedGroupedCS.clear();
    UpdateActiveControlSurfVec();
}

void VSPAEROMgrSingleton::RemoveSelectedFromCSGroup()
{
    vector < int > selected = m_SelectedGroupedCS;
    if ( m_CurrentCSGroupIndex != -1 )
    {
        for ( size_t i = 0; i < selected.size(); ++i )
        {
            m_ControlSurfaceGroupVec[ m_CurrentCSGroupIndex ]->RemoveSubSurface( m_ActiveControlSurfaceVec[selected[i] - 1].SSID,
                    m_ActiveControlSurfaceVec[selected[i] - 1].iReflect );
            for ( size_t j = 0; j < m_CompleteControlSurfaceVec.size(); ++j )
            {
                if ( m_ActiveControlSurfaceVec[selected[i] - 1].SSID.compare( m_CompleteControlSurfaceVec[j].SSID ) == 0 )
                {
                    if ( m_ActiveControlSurfaceVec[selected[i] - 1].iReflect == m_CompleteControlSurfaceVec[j].iReflect )
                    {
                        m_CompleteControlSurfaceVec[ j ].isGrouped = false;
                    }
                }
            }
        }
    }
    m_SelectedGroupedCS.clear();
    UpdateActiveControlSurfVec();
}

void VSPAEROMgrSingleton::RemoveAllFromCSGroup()
{
    if ( m_CurrentCSGroupIndex != -1 )
    {
        for ( size_t i = 0; i < m_ActiveControlSurfaceVec.size(); ++i )
        {
            m_ControlSurfaceGroupVec[ m_CurrentCSGroupIndex ]->RemoveSubSurface( m_ActiveControlSurfaceVec[i].SSID, m_ActiveControlSurfaceVec[i].iReflect );
            for ( size_t j = 0; j < m_CompleteControlSurfaceVec.size(); ++j )
            {
                if ( m_ActiveControlSurfaceVec[i].SSID.compare( m_CompleteControlSurfaceVec[j].SSID ) == 0 )
                {
                    if ( m_ActiveControlSurfaceVec[i].iReflect == m_CompleteControlSurfaceVec[j].iReflect )
                    {
                        m_CompleteControlSurfaceVec[ j ].isGrouped = false;
                    }
                }
            }
        }
    }
    m_SelectedGroupedCS.clear();
    UpdateActiveControlSurfVec();
}

string VSPAEROMgrSingleton::GetCurrentCSGGroupName()
{
    if ( m_CurrentCSGroupIndex != -1 )
    {
        return m_ControlSurfaceGroupVec[ m_CurrentCSGroupIndex ]->GetName();
    }
    else
    {
        return "";
    }
}

vector < VspAeroControlSurf > VSPAEROMgrSingleton::GetAvailableCSVec()
{
    vector < VspAeroControlSurf > ungrouped_cs;

    for ( size_t i = 0; i < m_CompleteControlSurfaceVec.size(); i++ )
    {
        bool grouped = false;

        for ( size_t j = 0; j < m_ActiveControlSurfaceVec.size(); j++ )
        {
            if ( ( strcmp( m_CompleteControlSurfaceVec[i].SSID.c_str(), m_ActiveControlSurfaceVec[j].SSID.c_str() ) == 0 ) && 
                 ( m_CompleteControlSurfaceVec[i].iReflect == m_ActiveControlSurfaceVec[j].iReflect ) &&
                 ( strcmp( m_CompleteControlSurfaceVec[i].fullName.c_str(), m_ActiveControlSurfaceVec[j].fullName.c_str() ) == 0 ) )
            {
                grouped = true;
                break;
            }
        }

        if ( !grouped )
        {
            ungrouped_cs.push_back( m_CompleteControlSurfaceVec[i] );
        }
    }

    return ungrouped_cs;
}

void VSPAEROMgrSingleton::SetCurrentCSGroupName( const string & name )
{
    if ( m_CurrentCSGroupIndex != -1 )
    {
        m_ControlSurfaceGroupVec[ m_CurrentCSGroupIndex ]->SetName( name );
    }
}

void VSPAEROMgrSingleton::HighlightSelected( int type )
{
    Vehicle* veh = VehicleMgr.GetVehicle();

    veh->ClearActiveGeom();

    if ( type == ROTORDISK )
    {
        VSPAEROMgr.SetCurrentType( ROTORDISK );
    }
    else if ( type == CONTROL_SURFACE )
    {
        VSPAEROMgr.SetCurrentType( CONTROL_SURFACE );
    }
    else
    {
        return;
    }
}

void VSPAEROMgrSingleton::LoadDrawObjs( vector < DrawObj* > & draw_obj_vec )
{
    Vehicle* veh = VehicleMgr.GetVehicle();
    if ( !veh )
    {
        return;
    }

    if ( m_LastSelectedType == ROTORDISK )
    {
        UpdateBBox( draw_obj_vec );
    }
    else if ( m_LastSelectedType == CONTROL_SURFACE )
    {
        UpdateHighlighted( draw_obj_vec );
    }

    for ( size_t i = 0; i < m_CpSliceVec.size(); i++ )
    {
        bool highlight = false;
        if ( m_CurrentCpSliceIndex == i )
        {
            highlight = true;
        }

        m_CpSliceVec[i]->LoadDrawObj( draw_obj_vec, i, highlight );
    }
}

void VSPAEROMgrSingleton::UpdateBBox( vector < DrawObj* > & draw_obj_vec )
{
    Vehicle* veh = VehicleMgr.GetVehicle();
    if ( !veh )
    {
        return;
    }

    //==== Load Bounding Box ====//
    m_BBox.Reset();
    BndBox bb;

    // If there is no selected rotor size is zero ( like blank geom )
    // set bbox to zero size
    if ( m_CurrentRotorDiskIndex == -1 )
    {
        m_BBox.Update( vec3d( 0, 0, 0 ) );
    }
    else
    {
        vector < VspSurf > surf_vec;
        Geom* geom = veh->FindGeom( m_RotorDiskVec[m_CurrentRotorDiskIndex]->GetParentID() );
        if ( geom )
        {
            geom->GetSurfVec( surf_vec );
            surf_vec[m_RotorDiskVec[m_CurrentRotorDiskIndex]->GetSurfNum()].GetBoundingBox( bb );
            m_BBox.Update( bb );
        }
        else
        { m_CurrentRotorDiskIndex = -1; }
    }

    //==== Bounding Box ====//
    m_HighlightDrawObj.m_Screen = DrawObj::VSP_MAIN_SCREEN;
    m_HighlightDrawObj.m_GeomID = BBOXHEADER + m_ID;
    m_HighlightDrawObj.m_LineWidth = 2.0;
    m_HighlightDrawObj.m_LineColor = vec3d( 1.0, 0., 0.0 );
    m_HighlightDrawObj.m_Type = DrawObj::VSP_LINES;

    m_HighlightDrawObj.m_PntVec = m_BBox.GetBBoxDrawLines();

    draw_obj_vec.push_back( &m_HighlightDrawObj );
}

void VSPAEROMgrSingleton::UpdateHighlighted( vector < DrawObj* > & draw_obj_vec )
{
    Vehicle* veh = VehicleMgr.GetVehicle();
    if ( !veh )
    {
        return;
    }

    string parentID = "";
    string ssid = "";
    int sub_surf_indx;
    if ( m_CurrentCSGroupIndex != -1 )
    {
        vector < VspAeroControlSurf > cont_surf_vec = m_ActiveControlSurfaceVec;
        vector < VspAeroControlSurf > cont_surf_vec_ungrouped = GetAvailableCSVec();
        if ( m_SelectedGroupedCS.size() == 0 && m_SelectedUngroupedCS.size() == 0 )
        {
            for ( size_t i = 0; i < cont_surf_vec.size(); ++i )
            {
                vec3d color( 0, 1, 0 ); // Green
                parentID = cont_surf_vec[i].parentGeomId;
                sub_surf_indx = cont_surf_vec[i].iReflect;
                ssid = cont_surf_vec[i].SSID;
                Geom* geom = veh->FindGeom( parentID );
                if ( geom )
                {
                    SubSurface* subsurf = geom->GetSubSurf( ssid );
                    if ( subsurf )
                    {
                        subsurf->LoadPartialColoredDrawObjs( ssid, sub_surf_indx, draw_obj_vec, color );
                    }
                }
            }
        }
        else
        {
            if ( cont_surf_vec_ungrouped.size() > 0 )
            {
                for ( size_t i = 0; i < m_SelectedUngroupedCS.size(); ++i )
                {
                    vec3d color( 1, 0, 0 ); // Red
                    parentID = cont_surf_vec_ungrouped[m_SelectedUngroupedCS[i] - 1].parentGeomId;
                    sub_surf_indx = cont_surf_vec_ungrouped[m_SelectedUngroupedCS[i] - 1].iReflect;
                    ssid = cont_surf_vec_ungrouped[m_SelectedUngroupedCS[i] - 1].SSID;
                    Geom* geom = veh->FindGeom( parentID );
                    SubSurface* subsurf = geom->GetSubSurf( ssid );
                    if ( subsurf )
                    {
                        subsurf->LoadPartialColoredDrawObjs( ssid, sub_surf_indx, draw_obj_vec, color );
                    }
                }
            }

            if ( cont_surf_vec.size() > 0 )
            {
                for ( size_t i = 0; i < m_SelectedGroupedCS.size(); ++i )
                {
                    vec3d color( 0, 1, 0 ); // Green
                    parentID = cont_surf_vec[m_SelectedGroupedCS[i] - 1].parentGeomId;
                    sub_surf_indx = cont_surf_vec[m_SelectedGroupedCS[i] - 1].iReflect;
                    ssid = cont_surf_vec[m_SelectedGroupedCS[i] - 1].SSID;
                    Geom* geom = veh->FindGeom( parentID );
                    SubSurface* subsurf = geom->GetSubSurf( ssid );
                    if ( subsurf )
                    {
                        subsurf->LoadPartialColoredDrawObjs( ssid, sub_surf_indx, draw_obj_vec, color );
                    }
                }
            }
        }
    }
}

string VSPAEROMgrSingleton::ComputeCpSlices( FILE * logFile )
{
    string resID = string();

    // Save analysis type for Cp Slicer
    m_CpSliceAnalysisType = m_AnalysisMethod.Get();

    CreateCutsFile();

    resID = ExecuteCpSlicer( logFile );

    vector < string > resIDvec;
    ReadSliceFile( m_SliceFile, resIDvec );

    // Add Case Result IDs to CpSlice Wrapper Result
    Results* res = ResultsMgr.FindResultsPtr( resID );
    if ( res )
    {
        res->Add( NameValData( "CpSlice_Case_ID_Vec", resIDvec ) );
    }

    return resID;
}

string VSPAEROMgrSingleton::ExecuteCpSlicer( FILE * logFile )
{
    Vehicle* veh = VehicleMgr.GetVehicle();
    if ( !veh )
    {
        return string();
    }

    WaitForFile( m_AdbFile );
    if ( !FileExist( m_AdbFile ) )
    {
        fprintf( stderr, "WARNING: Aerothermal database file not found: %s\n\tFile: %s \tLine:%d\n", m_AdbFile.c_str(), __FILE__, __LINE__ );
    }

    WaitForFile( m_CutsFile );
    if ( !FileExist( m_CutsFile ) )
    {
        fprintf( stderr, "WARNING: Cuts file not found: %s\n\tFile: %s \tLine:%d\n", m_CutsFile.c_str(), __FILE__, __LINE__ );
    }

    //====== Send command to be executed by the system at the command prompt ======//
    vector<string> args;

    // Add model file name
    args.push_back( m_ModelNameBase );

    //====== Execute VSPAERO Slicer ======//
    m_SlicerThread.ForkCmd( veh->GetExePath(), veh->GetSLICERCmd(), args );

    // ==== MonitorSolverProcess ==== //
    MonitorSolver( logFile );

    // Write out new results
    Results* res = ResultsMgr.CreateResults( "CpSlice_Wrapper" );
    if ( !res )
    {
        fprintf( stderr, "ERROR: Unable to create result in result manager \n\tFile: %s \tLine:%d\n", __FILE__, __LINE__ );
        return string();
    }
    else
    {
        int num_slice = m_CpSliceVec.size();
        res->Add( NameValData( "Num_Cuts", num_slice ) );
    }

    return res->GetID();
}

void VSPAEROMgrSingleton::ClearCpSliceResults()
{
    // Clear previous results
    while ( ResultsMgr.GetNumResults( "CpSlicer_Case" ) > 0 )
    {
        ResultsMgr.DeleteResult( ResultsMgr.FindResultsID( "CpSlicer_Case", 0 ) );
    }
    while ( ResultsMgr.GetNumResults( "CpSlicer_Wrapper" ) > 0 )
    {
        ResultsMgr.DeleteResult( ResultsMgr.FindResultsID( "CpSlicer_Wrapper", 0 ) );
    }
}

void VSPAEROMgrSingleton::CreateCutsFile()
{
    Vehicle *veh = VehicleMgr.GetVehicle();
    if ( !veh )
    {
        fprintf( stderr, "ERROR %d: Unable to get vehicle \n\tFile: %s \tLine:%d\n", vsp::VSP_INVALID_PTR, __FILE__, __LINE__ );
        return ;
    }

    // Clear existing cuts file
    if ( FileExist( m_CutsFile ) )
    {
        remove( m_CutsFile.c_str() );
    }

    FILE * cut_file = fopen( m_CutsFile.c_str(), "w" );
    if ( cut_file == NULL )
    {
        fprintf( stderr, "ERROR %d: Unable to create cuts file: %s\n\tFile: %s \tLine:%d\n", vsp::VSP_INVALID_PTR, m_CutsFile.c_str(), __FILE__, __LINE__ );
        return;
    }

    int numcuts = m_CpSliceVec.size();

    fprintf( cut_file, "%d\n", numcuts );

    for ( size_t i = 0; i < numcuts; i++ )
    {
        fprintf( cut_file, "%c %f\n", 120 + m_CpSliceVec[i]->m_CutType(),
                 m_CpSliceVec[i]->m_CutPosition() );
    }

    //Finish up by closing the file and making sure that it appears in the file system
    fclose( cut_file );

    // Wait until the setup file shows up on the file system
    WaitForFile( m_SetupFile );

}

void VSPAEROMgrSingleton::AddCpSliceVec( int cut_type, vector< double > cut_vec )
{
    for ( size_t i = 0; i < cut_vec.size(); i++ )
    {
        CpSlice* slice = AddCpSlice();

        if ( slice )
        {
            slice->m_CutType.Set( cut_type );
            slice->m_CutPosition.Set( cut_vec[i] );
        }
    }
}

vector < double > VSPAEROMgrSingleton::GetCpSlicePosVec( int type )
{
    vector < double > cut_pos_vec;

    for ( size_t i = 0; i < m_CpSliceVec.size(); i++ )
    {
        if ( m_CpSliceVec[i]->m_CutType() == type )
        {
            cut_pos_vec.push_back( m_CpSliceVec[i]->m_CutPosition() );
        }
    }
    return cut_pos_vec;
}

bool VSPAEROMgrSingleton::ValidCpSliceInd( int ind )
{
    if ( (int)m_CpSliceVec.size() > 0 && ind >= 0 && ind < (int)m_CpSliceVec.size() )
    {
        return true;
    }
    else
    {
        return false;
    }
}

void VSPAEROMgrSingleton::DelCpSlice( int ind )
{
    if ( ValidCpSliceInd( ind ) )
    {
        delete m_CpSliceVec[ind];
        m_CpSliceVec.erase( m_CpSliceVec.begin() + ind );
    }
}

CpSlice* VSPAEROMgrSingleton::AddCpSlice( )
{
    CpSlice* slice = NULL;
    slice = new CpSlice();

    if ( slice )
    {
        slice->SetName( string( "CpSlice_" + to_string( (long long)m_CpSliceVec.size() ) ) );
        slice->SetParentContainer( GetID() );
        AddCpSlice( slice );
    }

    return slice;
}

CpSlice* VSPAEROMgrSingleton::GetCpSlice( int ind )
{
    if ( ValidCpSliceInd( ind ) )
    {
        return m_CpSliceVec[ind];
    }
    return NULL;
}

int VSPAEROMgrSingleton::GetCpSliceIndex( const string & id )
{
    for ( int i = 0; i < (int)m_CpSliceVec.size(); i++ )
    {
        if ( m_CpSliceVec[i]->GetID() == id && ValidCpSliceInd( i ) )
        {
            return i;
        }
    }
    return -1;
}

void VSPAEROMgrSingleton::ClearCpSliceVec()
{
    for ( size_t i = 0; i < m_CpSliceVec.size(); ++i )
    {
        delete m_CpSliceVec[i];
        m_CpSliceVec.erase( m_CpSliceVec.begin() + i );
    }
    m_CpSliceVec.clear();
}

void VSPAEROMgrSingleton::ReadSliceFile( string filename, vector <string> &res_id_vector )
{
    FILE *fp = NULL;
    bool read_success = false;
    WaitForFile( filename );
    fp = fopen( filename.c_str(), "r" );
    if ( fp == NULL )
    {
        fprintf( stderr, "ERROR %d: Could not open Slice file: %s\n\tFile: %s \tLine:%d\n", vsp::VSP_FILE_DOES_NOT_EXIST, m_SliceFile.c_str(), __FILE__, __LINE__ );
        return;
    }

    Results* res = NULL;
    std::vector<string> data_string_array;
    int num_table_columns = 4;

    // Read in all of the data into the results manager
    char seps[] = " :,_\t\n";
    bool skip = false;

    while ( !feof( fp ) )
    {
        if ( !skip )
        {
            data_string_array = ReadDelimLine( fp, seps ); //this is also done in some of the embedded loops below
        }
        skip = false;

        if ( data_string_array.size() > 0 )
        {
            if ( strcmp( data_string_array[0].c_str(), "BLOCK" ) == 0 )
            {
                res = ResultsMgr.CreateResults( "CpSlicer_Case" );
                res_id_vector.push_back( res->GetID() );

                res->Add( NameValData( "Cut_Type", (int)( data_string_array[4][0] - 88 ) ) ); // ASCII X: 88; Y: 89; Z: 90
                res->Add( NameValData( "Cut_Loc", std::stod( data_string_array[5] ) ) );
                res->Add( NameValData( "Cut_Num", std::stoi( data_string_array[2] ) ) );
            }
            else if ( res && strcmp( data_string_array[0].c_str(), "Case" ) == 0 )
            {
                res->Add( NameValData( "Case", std::stoi( data_string_array[1] ) ) );
                res->Add( NameValData( "Mach", std::stod( data_string_array[4] ) ) );
                res->Add( NameValData( "Alpha", std::stod( data_string_array[7] ) ) );
                res->Add( NameValData( "Beta", std::stod( data_string_array[10] ) ) );
            }
            //READ slc table
            /* Example slc table
            BLOCK Cut_1_at_X:_2.000000
            Case: 1 ... Mach: 0.001000 ... Alpha: 1.000000 ... Beta: 0.000000 ...     Case: 1 ...
            x          y          z         dCp/Cp
            2.0000     0.0000    -0.6063    -0.0000
            2.0000     0.0000    -0.5610    -0.0000
            2.0000     0.0000    -0.4286    -0.0000
            2.0000     0.0000    -0.1093    -0.0000
            */
            else if ( res && data_string_array.size() == num_table_columns && strcmp( data_string_array[0].c_str(), "x" ) != 0 )
            {
                // create new vectors for this set of results information
                vector < double > x_data_vec, y_data_vec, z_data_vec, Cp_data_vec;

                while ( data_string_array.size() == num_table_columns )
                {
                    x_data_vec.push_back( std::stod( data_string_array[0] ) );
                    y_data_vec.push_back( std::stod( data_string_array[1] ) );
                    z_data_vec.push_back( std::stod( data_string_array[2] ) );
                    Cp_data_vec.push_back( std::stod( data_string_array[3] ) );

                    data_string_array = ReadDelimLine( fp, seps );
                }

                skip = true;

                //Add to the results manager
                res->Add( NameValData( "X_Loc", x_data_vec ) );
                res->Add( NameValData( "Y_Loc", y_data_vec ) );
                res->Add( NameValData( "Z_Loc", z_data_vec ) );

                if ( m_CpSliceAnalysisType == vsp::VORTEX_LATTICE )
                {
                    res->Add( NameValData( "dCp", Cp_data_vec ) );
                }
                else if ( m_CpSliceAnalysisType == vsp::PANEL )
                {
                    res->Add( NameValData( "Cp", Cp_data_vec ) );
                }
            } // end of cut data
        }
    }

    std::fclose( fp );

    return;
}

/*##############################################################################
#                                                                              #
#                               CpSlice                                        #
#                                                                              #
##############################################################################*/

CpSlice::CpSlice() : ParmContainer()
{
    m_CutType.Init( "CutType", "CpSlice", this, vsp::Y_DIR, vsp::X_DIR, vsp::Z_DIR );
    m_CutType.SetDescript( "Perpendicular Axis for the Cut" );

    m_CutPosition.Init( "CutPosition", "CpSlice", this, 0.0, -1e12, 1e12 );
    m_CutPosition.SetDescript( "Position of the Cut from Orgin Along Perpendicular Axis" );

    m_DrawCutFlag.Init( "DrawCutFlag", "CpSlice", this, true, false, true );
    m_DrawCutFlag.SetDescript( "Flag to Draw the CpSlice Cutting Plane" );
}

CpSlice::~CpSlice( )
{

}

void CpSlice::ParmChanged( Parm* parm_ptr, int type )
{
    if ( type == Parm::SET )
    {
        m_LateUpdateFlag = true;
        return;
    }

    Vehicle* veh = VehicleMgr.GetVehicle();
    if ( veh )
    {
        veh->ParmChanged( parm_ptr, type );
    }
}

VspSurf CpSlice::CreateSurf()
{
    VspSurf slice_surf = VspSurf();

    Vehicle* veh = VehicleMgr.GetVehicle();
    if ( veh )
    {
        vec3d pnt0, pnt1, pnt2, pnt3;

        vec3d max_pnt = veh->GetBndBox().GetMax();
        vec3d min_pnt = veh->GetBndBox().GetMin();
        double del_x = ( max_pnt.x() - min_pnt.x() ) / 2;
        double del_y = ( max_pnt.y() - min_pnt.y() ) / 2;
        double del_z = ( max_pnt.z() - min_pnt.z() ) / 2;

        vec3d veh_center = veh->GetBndBox().GetCenter();

        // Center at vehicle bounding box center
        if ( m_CutType() == vsp::X_DIR )
        {
            pnt0 = vec3d( m_CutPosition(), del_y + veh_center.y(), del_z + veh_center.z() );
            pnt1 = vec3d( m_CutPosition(), -1 * del_y + veh_center.y(), del_z + veh_center.z() );
            pnt2 = vec3d( m_CutPosition(), del_y + veh_center.y(), -1 * del_z + veh_center.z() );
            pnt3 = vec3d( m_CutPosition(), -1 * del_y + veh_center.y(), -1 * del_z + veh_center.z() );
        }
        else if ( m_CutType() == vsp::Y_DIR )
        {
            pnt0 = vec3d(del_x + veh_center.x(), m_CutPosition(), del_z + veh_center.z() );
            pnt1 = vec3d( -1 * del_x + veh_center.x(), m_CutPosition(), del_z + veh_center.z() );
            pnt2 = vec3d(del_x + veh_center.x(), m_CutPosition(), -1 * del_z + veh_center.z() );
            pnt3 = vec3d( -1 * del_x + veh_center.x(), m_CutPosition(), -1 * del_z + veh_center.z() );
        }
        else if ( m_CutType() == vsp::Z_DIR )
        {
            pnt0 = vec3d(del_x + veh_center.x(), del_y + veh_center.y(), m_CutPosition() );
            pnt1 = vec3d( -1 * del_x + veh_center.x(), del_y + veh_center.y(), m_CutPosition() );
            pnt2 = vec3d(del_x + veh_center.x(), -1 * del_y + veh_center.y(), m_CutPosition() );
            pnt3 = vec3d( -1 * del_x + veh_center.x(), -1 * del_y + veh_center.y(), m_CutPosition() );
        }

        slice_surf.MakePlaneSurf( pnt0, pnt1, pnt2, pnt3 );
    }

    return slice_surf;
}

void CpSlice::LoadDrawObj( vector < DrawObj* > &draw_obj_vec, int id, bool highlight )
{
    // One DrawObj for plane and one for border. This is done to avoid DrawObj ordering transparancy issues
    m_CpSliceDOVec.clear();
    m_CpSliceDOVec.resize( 2 );

    if ( m_DrawCutFlag() )
    {
        VspSurf slice_surf = CreateSurf();

        m_CpSliceDOVec[0].m_GeomID = m_Name + "_Plane_" + std::to_string( id );
        m_CpSliceDOVec[0].m_Screen = DrawObj::VSP_MAIN_SCREEN;

        m_CpSliceDOVec[1].m_GeomID = m_Name + "_Border_" + std::to_string( id );
        m_CpSliceDOVec[1].m_Screen = DrawObj::VSP_MAIN_SCREEN;

        if ( highlight )
        {
            m_CpSliceDOVec[1].m_LineColor = vec3d( 1.0, 0.0, 0.0 );
            m_CpSliceDOVec[1].m_LineWidth = 3.0;
        }
        else
        {
            m_CpSliceDOVec[1].m_LineColor = vec3d( 96.0 / 255.0, 96.0 / 255.0, 96.0 / 255.0 );
            m_CpSliceDOVec[1].m_LineWidth = 1.0;
        }

        m_CpSliceDOVec[0].m_Type = DrawObj::VSP_SHADED_QUADS;
        m_CpSliceDOVec[1].m_Type = DrawObj::VSP_LINE_LOOP;

        vec3d p00 = slice_surf.CompPnt01( 0, 0 );
        vec3d p10 = slice_surf.CompPnt01( 1, 0 );
        vec3d p11 = slice_surf.CompPnt01( 1, 1 );
        vec3d p01 = slice_surf.CompPnt01( 0, 1 );

        m_CpSliceDOVec[0].m_PntVec.push_back( p00 );
        m_CpSliceDOVec[0].m_PntVec.push_back( p10 );
        m_CpSliceDOVec[0].m_PntVec.push_back( p11 );
        m_CpSliceDOVec[0].m_PntVec.push_back( p01 );

        m_CpSliceDOVec[1].m_PntVec.push_back( p00 );
        m_CpSliceDOVec[1].m_PntVec.push_back( p10 );
        m_CpSliceDOVec[1].m_PntVec.push_back( p11 );
        m_CpSliceDOVec[1].m_PntVec.push_back( p01 );

        // Get new normal and set plane color to medium glass
        vec3d quadnorm = cross( p10 - p00, p01 - p00 );
        quadnorm.normalize();

        for ( size_t i = 0; i < 4; i++ )
        {
            m_CpSliceDOVec[0].m_MaterialInfo.Ambient[i] = 0.2f;
            m_CpSliceDOVec[0].m_MaterialInfo.Diffuse[i] = 0.1f;
            m_CpSliceDOVec[0].m_MaterialInfo.Specular[i] = 0.7f;
            m_CpSliceDOVec[0].m_MaterialInfo.Emission[i] = 0.0f;

            m_CpSliceDOVec[0].m_NormVec.push_back( quadnorm );
        }

        if ( highlight )
        {
            m_CpSliceDOVec[0].m_MaterialInfo.Diffuse[3] = 0.67f;
        }
        else
        {
            m_CpSliceDOVec[0].m_MaterialInfo.Diffuse[3] = 0.33f;
        }

        m_CpSliceDOVec[0].m_MaterialInfo.Shininess = 5.0f;

        m_CpSliceDOVec[0].m_GeomChanged = true;
        draw_obj_vec.push_back( &m_CpSliceDOVec[0] );

        m_CpSliceDOVec[1].m_GeomChanged = true;
        draw_obj_vec.push_back( &m_CpSliceDOVec[1] );
    }
}

/*##############################################################################
#                                                                              #
#                              RotorDisk                                       #
#                                                                              #
##############################################################################*/

RotorDisk::RotorDisk( void ) : ParmContainer()
{
    m_Name = "Default";
    m_GroupName = "Rotor";

    m_IsUsed = true;

    m_XYZ.set_xyz( 0, 0, 0 );           // RotorXYZ_
    m_Normal.set_xyz( 0, 0, 0 );        // RotorNormal_

    m_Diameter.Init( "RotorDiameter", m_GroupName, this, 10.0, 0.0, 1e12 );       // RotorDiameter_
    m_Diameter.SetDescript( "Rotor Diameter" );

    m_HubDiameter.Init( "RotorHubDiameter", m_GroupName, this, 0.0, 0.0, 1e12 );    // RotorHubDiameter_
    m_HubDiameter.SetDescript( "Rotor Hub Diameter" );

    m_RPM.Init( "RotorRPM", m_GroupName, this, 2000.0, -1e12, 1e12 );       // RotorRPM_
    m_RPM.SetDescript( "Rotor RPM" );

    m_CT.Init( "RotorCT", m_GroupName, this, 0.4, -1e3, 1e3 );       // Rotor_CT_
    m_CT.SetDescript( "Rotor Coefficient of Thrust" );

    m_CP.Init( "RotorCP", m_GroupName, this, 0.6, -1e3, 1e3 );        // Rotor_CP_
    m_CP.SetDescript( "Rotor Coefficient of Power" );

    m_ParentGeomId = "";
    m_ParentGeomSurfNdx = -1;
}


RotorDisk::~RotorDisk( void )
{
}

void RotorDisk::ParmChanged( Parm* parm_ptr, int type )
{
    Vehicle* veh = VehicleMgr.GetVehicle();

    if ( veh )
    {
        veh->ParmChanged( parm_ptr, type );
    }
}

RotorDisk& RotorDisk::operator=( const RotorDisk &RotorDisk )
{

    m_Name = RotorDisk.m_Name;

    m_XYZ = RotorDisk.m_XYZ;           // RotorXYZ_
    m_Normal = RotorDisk.m_Normal;        // RotorNormal_

    m_Diameter = RotorDisk.m_Diameter;       // RotorRadius_
    m_HubDiameter = RotorDisk.m_HubDiameter;    // RotorHubRadius_
    m_RPM = RotorDisk.m_RPM;       // RotorRPM_

    m_CT = RotorDisk.m_CT;        // Rotor_CT_
    m_CP = RotorDisk.m_CP;        // Rotor_CP_

    m_ParentGeomId = RotorDisk.m_ParentGeomId;
    m_ParentGeomSurfNdx = RotorDisk.m_ParentGeomSurfNdx;

    return *this;

}

void RotorDisk::Write_STP_Data( FILE *InputFile )
{

    // Write out RotorDisk to file

    fprintf( InputFile, "%lf %lf %lf \n", m_XYZ.x(), m_XYZ.y(), m_XYZ.z() );

    fprintf( InputFile, "%lf %lf %lf \n", m_Normal.x(), m_Normal.y(), m_Normal.z() );

    fprintf( InputFile, "%lf \n", m_Diameter() / 2.0 );

    fprintf( InputFile, "%lf \n", m_HubDiameter() / 2.0 );

    fprintf( InputFile, "%lf \n", m_RPM() );

    fprintf( InputFile, "%lf \n", m_CT() );

    fprintf( InputFile, "%lf \n", m_CP() );

}

xmlNodePtr RotorDisk::EncodeXml( xmlNodePtr & node )
{
    if ( node )
    {
        ParmContainer::EncodeXml( node );
        XmlUtil::AddStringNode( node, "ParentID", m_ParentGeomId.c_str() );
        XmlUtil::AddIntNode( node, "SurfIndex", m_ParentGeomSurfNdx );
    }

    return node;
}

xmlNodePtr RotorDisk::DecodeXml( xmlNodePtr & node )
{
    string defstr = "";
    int defint = 0;
    if ( node )
    {
        ParmContainer::DecodeXml( node );
        m_ParentGeomId = XmlUtil::FindString( node, "ParentID", defstr );
        m_ParentGeomSurfNdx = XmlUtil::FindInt( node, "SurfIndex", defint );
    }

    return node;
}

void RotorDisk::SetGroupDisplaySuffix(int num)
{
    m_GroupSuffix = num;
    //==== Assign Group Suffix To All Parms ====//
    for ( int i = 0 ; i < ( int )m_ParmVec.size() ; i++ )
    {
        Parm* p = ParmMgr.FindParm( m_ParmVec[i] );
        if ( p )
        {
            p->SetGroupDisplaySuffix( num );
        }
    }
}

/*##############################################################################
#                                                                              #
#                        ControlSurfaceGroup                                   #
#                                                                              #
##############################################################################*/

ControlSurfaceGroup::ControlSurfaceGroup( void ) : ParmContainer()
{
    m_Name = "Unnamed Control Group";
    m_ParentGeomBaseID = "";

    m_GroupName = "ControlSurfaceGroup";

    m_IsUsed.Init( "ActiveFlag", m_GroupName, this, true, false, true );
    m_IsUsed.SetDescript( "Flag to determine whether or not this group will be used in VSPAero" );

    m_DeflectionAngle.Init( "DeflectionAngle", m_GroupName, this, 0.0, -1.0e12, 1.0e12 );
    m_DeflectionAngle.SetDescript( "Angle of deflection for the control group" );
}

ControlSurfaceGroup::~ControlSurfaceGroup( void )
{
}

void ControlSurfaceGroup::ParmChanged( Parm* parm_ptr, int type )
{
    Vehicle* veh = VehicleMgr.GetVehicle();

    if ( veh )
    {
        veh->ParmChanged( parm_ptr, type );
    }
}

void ControlSurfaceGroup::Write_STP_Data( FILE *InputFile )
{
    Vehicle* veh = VehicleMgr.GetVehicle();
    if ( !veh )
    {
        return;
    }

    string nospacename;

    // Write out Control surface group to .vspaero file
    nospacename = m_Name;
    StringUtil::chance_space_to_underscore( nospacename );
    fprintf( InputFile, "%s\n", nospacename.c_str() );

    // surface names ( Cannot have trailing commas )
    unsigned int i = 0;
    for ( i = 0; i < m_ControlSurfVec.size() - 1; i++ )
    {
        nospacename = m_ControlSurfVec[i].fullName;
        StringUtil::chance_space_to_underscore( nospacename );
        fprintf( InputFile, "%s,", nospacename.c_str() );
    }
    nospacename = m_ControlSurfVec[i++].fullName;
    StringUtil::chance_space_to_underscore( nospacename );
    fprintf( InputFile, "%s\n", nospacename.c_str() );

    // deflection mixing gains ( Cannot have trailing commas )
    for ( i = 0; i < m_DeflectionGainVec.size() - 1; i++ )
    {
        fprintf( InputFile, "%lg, ", m_DeflectionGainVec[i]->Get() );
    }
    fprintf( InputFile, "%lg\n", m_DeflectionGainVec[i++]->Get() );

    // group deflection angle
    fprintf( InputFile, "%lg\n", m_DeflectionAngle() );

}

void ControlSurfaceGroup::Load_STP_Data( FILE *InputFile )
{
    //TODO - need to write function to load data from .vspaero file
}

xmlNodePtr ControlSurfaceGroup::EncodeXml( xmlNodePtr & node )
{
    if ( node )
    {
        XmlUtil::AddStringNode( node, "ParentGeomBase", m_ParentGeomBaseID.c_str() );

        XmlUtil::AddIntNode( node, "NumberOfControlSubSurfaces", m_ControlSurfVec.size() );
        for ( size_t i = 0; i < m_ControlSurfVec.size(); ++i )
        {
            xmlNodePtr csnode = xmlNewChild( node, NULL, BAD_CAST "Control_Surface" , NULL );

            XmlUtil::AddStringNode( csnode, "SSID", m_ControlSurfVec[i].SSID.c_str() );
            XmlUtil::AddStringNode( csnode, "ParentGeomID", m_ControlSurfVec[i].parentGeomId.c_str() );
            XmlUtil::AddIntNode( csnode, "iReflect", m_ControlSurfVec[i].iReflect );
        }

        ParmContainer::EncodeXml( node );
    }

    return node;
}

xmlNodePtr ControlSurfaceGroup::DecodeXml( xmlNodePtr & node )
{
    unsigned int nControlSubSurfaces = 0;
    string GroupName;
    string ParentGeomID;
    string SSID;

    int iReflect = 0;
    VspAeroControlSurf newSurf;

    if ( node )
    {
        m_ParentGeomBaseID = XmlUtil::FindString( node, "ParentGeomBase", ParentGeomID );

        nControlSubSurfaces = XmlUtil::FindInt( node, "NumberOfControlSubSurfaces", nControlSubSurfaces );
        for ( size_t i = 0; i < nControlSubSurfaces; ++i )
        {
            xmlNodePtr csnode = XmlUtil::GetNode( node, "Control_Surface", i );

            newSurf.SSID = XmlUtil::FindString( csnode, "SSID", SSID );
            newSurf.parentGeomId = XmlUtil::FindString( csnode, "ParentGeomID", ParentGeomID );
            newSurf.iReflect = XmlUtil::FindInt( csnode, "iReflect", iReflect );
            AddSubSurface( newSurf );
        }

        ParmContainer::DecodeXml( node ); // Comes after AddSubSurface() to prevent overwriting of newly initialized Parms
    }

    return node;
}

void ControlSurfaceGroup::AddSubSurface( VspAeroControlSurf control_surf )
{
    // Add deflection gain parm to ControlSurfaceGroup container
    Parm* p = ParmMgr.CreateParm( vsp::PARM_DOUBLE_TYPE );
    char str[256];

    if ( p )
    {
        //  parm name: control_surf->fullName (example: MainWing_Surf1_Aileron)
        //  group: "ControlSurfaceGroup"
        //  initial value: control_surf->deflection_gain
        sprintf( str, "Surf_%s_%i_Gain", control_surf.SSID.c_str(), control_surf.iReflect );
        p->Init( str, m_GroupName, this, 1.0, -1.0e6, 1.0e6 );
        p->SetDescript( "Deflection gain for the individual sub surface to be used for control mixing and allocation within the control surface group" );
        m_DeflectionGainVec.push_back( p );
    }

    m_ControlSurfVec.push_back( control_surf );
}

void ControlSurfaceGroup::RemoveSubSurface( const string & ssid, int reflec_num )
{
    for ( size_t i = 0; i < m_ControlSurfVec.size(); ++i )
    {
        if ( m_ControlSurfVec[i].SSID.compare( ssid ) == 0 && m_ControlSurfVec[i].iReflect == reflec_num )
        {
            m_ControlSurfVec.erase( m_ControlSurfVec.begin() + i );
            delete m_DeflectionGainVec[i];
            m_DeflectionGainVec.erase( m_DeflectionGainVec.begin() + i );
            return;
        }
    }
}

void ControlSurfaceGroup::SetGroupDisplaySuffix( int num )
{
    m_GroupSuffix = num;
    //==== Assign Group Suffix To All Parms ====//
    for ( int i = 0 ; i < ( int )m_ParmVec.size() ; i++ )
    {
        Parm* p = ParmMgr.FindParm( m_ParmVec[i] );
        if ( p )
        {
            p->SetGroupDisplaySuffix( num );
        }
    }
}