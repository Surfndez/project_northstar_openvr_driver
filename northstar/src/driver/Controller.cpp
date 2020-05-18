#include "driver/Controller.hpp"

using namespace northstar::driver::types;
using namespace northstar::driver::settings::values;
using namespace northstar::driver::settings::keys;
using namespace northstar::math::types;

northstar::driver::CController::CController(
    std::shared_ptr<northstar::driver::ISensorFrameCoordinator> pSensorFrameCoordinator,
    std::shared_ptr<northstar::math::IWorldAdapter> pWorldAdapter,
    std::shared_ptr<northstar::math::IVectorFactory> pVectorFactory,
    std::shared_ptr<northstar::math::ISkeletalAdapter> pSkeletalAdapter,
    std::shared_ptr<northstar::utility::ILogger> pLogger, 
    std::shared_ptr<northstar::openvr::IVRProperties> pVRProperties,
    vr::IVRSettings* pVRSettings,
    vr::IVRServerDriverHost* pVRServerDriverHost,
    vr::IVRDriverInput* pVRDriverInput,
    const EHand& eHand) {
    m_pLogger = pLogger;
    m_eHand = eHand;
    m_pWorldAdapter = pWorldAdapter;
    m_pVectorFactory = pVectorFactory;
    m_pSkeletalAdapter = pSkeletalAdapter;
    m_pSensorFrameCoordinator = pSensorFrameCoordinator;
    m_pVRProperties = pVRProperties;
    m_pVRSettings = pVRSettings;
    m_pVRServerDriverHost = pVRServerDriverHost;
    m_pVRDriverInput = pVRDriverInput;
    ClearOpenVRState();
    LoadConfiguration();
}

void northstar::driver::CController::ClearOpenVRState() {
    m_sOpenVRState.asOpenVRSkeletalFrameData = { 0 };
    m_sOpenVRState.bNewSkeletalFrameData = false;
    m_sOpenVRState.bSkeletalFrameDataInitialPoseSubmitted = false;
	m_sOpenVRState.unObjectId = vr::k_unTrackedDeviceIndexInvalid;
    m_sOpenVRState.ulPropertyContainer = vr::k_ulInvalidPropertyContainer;
    m_sOpenVRState.unTriggerValueComponent = vr::k_ulInvalidInputComponentHandle;
    m_sOpenVRState.unAClickComponent = vr::k_ulInvalidInputComponentHandle;
    m_sOpenVRState.unBClickComponent = vr::k_ulInvalidInputComponentHandle;
    m_sOpenVRState.unSystemClickComponent = vr::k_ulInvalidInputComponentHandle;
    m_sOpenVRState.unSkeletalComponent = vr::k_ulInvalidInputValueHandle;
    m_sOpenVRState.bPointerClick = false;
    m_sOpenVRState.bMiddleClick = false;
    m_sOpenVRState.bRingClick = false;
    m_sOpenVRState.bPinkyClick = false;
    m_sOpenVRState.fTriggerValue = 0.0f;
}

void northstar::driver::CController::LoadConfiguration() {
    m_sConfiguration.bUseDebugTrackerConfig = m_pVRSettings->GetBool(debug::k_svRoot.data(), debug::k_svUseDebugTrackerConfig.data());
    m_sConfiguration.bUseDebugBasePose = m_pVRSettings->GetBool(debug::k_svRoot.data(), debug::k_svUseDebugBasePose.data());
    m_sConfiguration.bUseRenderModel = m_pVRSettings->GetBool(configuration::k_svRoot.data(), configuration::k_svUseRenderModel.data());

    if (m_sConfiguration.bUseDebugTrackerConfig) {
        m_sConfiguration.sLeapMotionConfiguration.v3dPosition = m_pVectorFactory->V3DFromArray({ 0.0, 0.0, 0.0 });
        m_sConfiguration.sLeapMotionConfiguration.qdOrientation = Quaterniond(1.0, 0.0, 0.0, 0.0);
        return;
    }

    m_sConfiguration.sLeapMotionConfiguration.v3dPosition =
        m_pWorldAdapter->FromUnityPositionToOpenVRPosition(
            m_pVectorFactory->V3DFromArray(
                { m_pVRSettings->GetFloat(leapMotion::k_svRoot.data(), leapMotion::k_svPositionX.data())
                , m_pVRSettings->GetFloat(leapMotion::k_svRoot.data(), leapMotion::k_svPositionY.data())
                , m_pVRSettings->GetFloat(leapMotion::k_svRoot.data(), leapMotion::k_svPositionZ.data()) }
            )
        );

    m_sConfiguration.sLeapMotionConfiguration.qdOrientation =
        m_pWorldAdapter->FromUnityQuaternionToOpenVRQuaternion(
            Quaterniond(
                m_pVRSettings->GetFloat(leapMotion::k_svRoot.data(), leapMotion::k_svRotationW.data()),
                m_pVRSettings->GetFloat(leapMotion::k_svRoot.data(), leapMotion::k_svRotationX.data()),
                m_pVRSettings->GetFloat(leapMotion::k_svRoot.data(), leapMotion::k_svRotationY.data()),
                m_pVRSettings->GetFloat(leapMotion::k_svRoot.data(), leapMotion::k_svRotationZ.data())
            )
        );
}

vr::EVRInitError northstar::driver::CController::Activate(vr::TrackedDeviceIndex_t unObjectId) {
    m_sOpenVRState.unObjectId = unObjectId;
    m_sOpenVRState.ulPropertyContainer = m_pVRProperties->TrackedDeviceToPropertyContainer( unObjectId );
    SetOpenVRProperties();
    CreateOpenVRInputComponents();
    return vr::VRInitError_None;
}

void northstar::driver::CController::SetOpenVRProperties() {
    m_pVRProperties->SetStringProperty( m_sOpenVRState.ulPropertyContainer, vr::Prop_ModelNumber_String, x_svModelNumber.data() );
    m_pVRProperties->SetStringProperty( 
        m_sOpenVRState.ulPropertyContainer, 
        vr::Prop_SerialNumber_String, 
        m_eHand == EHand::Left ? x_svSerialNumberLeft.data() : x_svSerialNumberRight.data() );

    if (m_sConfiguration.bUseRenderModel) {
        m_pVRProperties->SetStringProperty(
            m_sOpenVRState.ulPropertyContainer,
            vr::Prop_RenderModelName_String,
            m_eHand == EHand::Left ? x_svRenderModelNameLeft.data() : x_svRenderModelNameRight.data());
    }

    m_pVRProperties->SetUint64Property( m_sOpenVRState.ulPropertyContainer, vr::Prop_CurrentUniverseId_Uint64, driverConfiguration::k_uiCurrentUniverseID);
    m_pVRProperties->SetInt32Property( 
        m_sOpenVRState.ulPropertyContainer, 
        vr::Prop_ControllerRoleHint_Int32, 
        m_eHand == EHand::Left ? vr::TrackedControllerRole_LeftHand : vr::TrackedControllerRole_RightHand);

    m_pVRProperties->SetStringProperty( m_sOpenVRState.ulPropertyContainer, vr::Prop_InputProfilePath_String, paths::k_svInputProfilePath.data());
    m_pVRProperties->SetStringProperty( m_sOpenVRState.ulPropertyContainer, vr::Prop_ControllerType_String, x_svControllerType.data() );
}

void northstar::driver::CController::CreateOpenVRInputComponents() {
    m_pVRDriverInput->CreateScalarComponent(
        m_sOpenVRState.ulPropertyContainer,
        paths::k_svTriggerValuePath.data(),
        &m_sOpenVRState.unTriggerValueComponent,
        vr::EVRScalarType::VRScalarType_Absolute,
        vr::EVRScalarUnits::VRScalarUnits_NormalizedOneSided);

    m_pVRDriverInput->CreateBooleanComponent(
        m_sOpenVRState.ulPropertyContainer,
        paths::k_svAClickPath.data(),
        &m_sOpenVRState.unAClickComponent);

    m_pVRDriverInput->CreateBooleanComponent(
        m_sOpenVRState.ulPropertyContainer,
        paths::k_svBClickPath.data(),
        &m_sOpenVRState.unBClickComponent);

    m_pVRDriverInput->CreateBooleanComponent(
        m_sOpenVRState.ulPropertyContainer,
        paths::k_svSystemClickPath.data(),
        &m_sOpenVRState.unSystemClickComponent);

    const auto eResult = m_pVRDriverInput->CreateSkeletonComponent(
        m_sOpenVRState.ulPropertyContainer,
        m_eHand == EHand::Left ? paths::k_svSkeletalInputPathNameLeft.data() : paths::k_svSkeletalInputPathNameRight.data(),
        m_eHand == EHand::Left ? paths::k_svSkeletalInputPathLeft.data() : paths::k_svSkeletalInputPathRight.data(),
        paths::k_svSkeletalInputPoseBasePath.data(),
        x_eSkeletalTrackingLevel,
        x_vrGripLimitTransforms,
        x_vrGripLimitTransformCount,
        &m_sOpenVRState.unSkeletalComponent);

    if (eResult != vr::EVRInputError::VRInputError_None) {
        m_pLogger->Log("Error creating skeletal component!");
    }
}

void northstar::driver::CController::Deactivate() {
    ClearOpenVRState();
}

void* northstar::driver::CController::GetComponent(const char* pchComponentNameAndVersion) {
    return nullptr;
}

const std::string_view& northstar::driver::CController::GetSerialNumber() const {
    return m_eHand == EHand::Left ? x_svSerialNumberLeft : x_svSerialNumberRight;
}

void northstar::driver::CController::EnterStandby() {}

void northstar::driver::CController::DebugRequest(const char* pchRequest, char* pchResponseBuffer, uint32_t unResponseBufferSize) {
    if ( unResponseBufferSize >= 1 )
        pchResponseBuffer[0] = 0;
}

// TODO: pose timing, use explicit or couple with hand data / confidence / visible time
// TODO: validate that bone orientation is as expect, looks like it is in OVRSpace in leap space
// TODO: better vec / velocity
// TODO: perf pass
vr::DriverPose_t northstar::driver::CController::GetPose() {
    vr::DriverPose_t sPose;
    auto osLeapHandData = m_pSensorFrameCoordinator->GetLeapHandPose(m_eHand);
    auto osDriverPoseData = m_pSensorFrameCoordinator->GetOpenVRHeadPose();
    sPose.deviceIsConnected = true;

    if (!osLeapHandData || !osDriverPoseData) {
        sPose.poseIsValid = false;
        sPose.result = vr::TrackingResult_Running_OutOfRange;
        return sPose;
    }

    sPose.poseIsValid = true;
    sPose.result = vr::TrackingResult_Running_OK;
    sPose.willDriftInYaw = false;
    sPose.shouldApplyHeadModel = false;

    // hands have no sensor offset or world offset
    sPose.qWorldFromDriverRotation = { 1, 0, 0, 0 };
    sPose.vecWorldFromDriverTranslation[0] = 0;
    sPose.vecWorldFromDriverTranslation[1] = 0;
    sPose.vecWorldFromDriverTranslation[2] = 0;

    sPose.qDriverFromHeadRotation = { 1, 0, 0, 0 };
    sPose.vecDriverFromHeadTranslation[0] = 0;
    sPose.vecDriverFromHeadTranslation[1] = 0;
    sPose.vecDriverFromHeadTranslation[2] = 0;

    static const AffineMatrix4d m4dLeapConversionMatrix = m_pWorldAdapter
        ->ConversionMatrixFromLeapMotionTrackingSpaceToHMDRelativeSpace(
            m_sConfiguration.sLeapMotionConfiguration.v3dPosition,
            m_sConfiguration.sLeapMotionConfiguration.qdOrientation);

    AffineMatrix4d m4dHMDToWorldSpace = m_pWorldAdapter
        ->ConversionMatrixFromHMDSpaceToOpenVRWorldSpace(
            osDriverPoseData.value());

    Vector4d v4dPosition = m4dHMDToWorldSpace 
        * m4dLeapConversionMatrix * m_pVectorFactory->V4DFromXYZWArray(
            { osLeapHandData.value().palm.position.x
            , osLeapHandData.value().palm.position.y
            , osLeapHandData.value().palm.position.z
            , 1 });

    sPose.vecPosition[0] = v4dPosition.x();
    sPose.vecPosition[1] = v4dPosition.y();
    sPose.vecPosition[2] = v4dPosition.z();

    Quaterniond orientation;
    orientation = m4dHMDToWorldSpace.rotation()
        * m4dLeapConversionMatrix.rotation()
        * Quaterniond(
            osLeapHandData.value().palm.orientation.w,
            osLeapHandData.value().palm.orientation.x,
            osLeapHandData.value().palm.orientation.y,
            osLeapHandData.value().palm.orientation.z);

    sPose.qRotation.w = orientation.w();
    sPose.qRotation.x = orientation.x();
    sPose.qRotation.y = orientation.y();
    sPose.qRotation.z = orientation.z();

    if (m_sConfiguration.bUseDebugBasePose) {
        if (m_eHand == EHand::Left) {
            sPose.vecPosition[0] = -0.3;
            sPose.vecPosition[1] = -0.2;
            sPose.vecPosition[2] = -0.4;

            sPose.qRotation.w = 1.0;
            sPose.qRotation.x = 0.0;
            sPose.qRotation.y = 0.0;
            sPose.qRotation.z = 0.0;
        }
        else {
            sPose.vecPosition[0] = 0.3;
            sPose.vecPosition[1] = -0.2;
            sPose.vecPosition[2] = -0.4;

            sPose.qRotation.w = 1.0;
            sPose.qRotation.x = 0.0;
            sPose.qRotation.y = 0.0;
            sPose.qRotation.z = 0.0;
        }
    }

    // TODO: This needs smoothing
    //Vector3d v4dTranslatedVelocity = m_pWorldAdapter
    //    ->FromLeapMotionVelocityToOpenVRVelocity(
    //        m4dLeapConversionMatrix,
    //        m_pVectorFactory->V3DFromArray(
    //            { osLeapHandData.value().palm.velocity.x
    //            , osLeapHandData.value().palm.velocity.y
    //            , osLeapHandData.value().palm.velocity.z }));

    sPose.vecVelocity[0] = 0;//v4dTranslatedVelocity.x();
    sPose.vecVelocity[1] = 0;//v4dTranslatedVelocity.y();
    sPose.vecVelocity[2] = 0;//v4dTranslatedVelocity.z();

    sPose.vecAcceleration[0] = 0;
    sPose.vecAcceleration[1] = 0;
    sPose.vecAcceleration[2] = 0;

    sPose.vecAngularVelocity[0] = 0;
    sPose.vecAngularVelocity[1] = 0;
    sPose.vecAngularVelocity[2] = 0;

    sPose.vecAngularAcceleration[0] = 0;
    sPose.vecAngularAcceleration[1] = 0;
    sPose.vecAngularAcceleration[2] = 0;

    sPose.poseTimeOffset = 0;
    UpdatePendingInputState(
        sPose,
        m4dLeapConversionMatrix,
        m4dHMDToWorldSpace,
        osLeapHandData.value());

    return sPose;
}

//TODO: move this to a gesture recognizer
void northstar::driver::CController::UpdatePendingInputState(
    const vr::DriverPose_t& sPose,
    const northstar::math::types::AffineMatrix4d& m4dFromLeapSensorToHMDRelativeSpace,
    const northstar::math::types::AffineMatrix4d& m4dFromHMDToWorldSpace,
    const LEAP_HAND& sLeapHand) {
    m_sOpenVRState.fTriggerValue = sLeapHand.pinch_strength;
    m_sOpenVRState.bPointerClick = false;
    //m_sOpenVRState.bPointerClick = EvaluateDigitProximityForClick(
    //    m_pVectorFactory->V3DFromArray(
    //        { static_cast<double>(sLeapHand.index.distal.next_joint.x)
    //        , static_cast<double>(sLeapHand.index.distal.next_joint.y)
    //        , static_cast<double>(sLeapHand.index.distal.next_joint.z) }),
    //    m_pVectorFactory->V3DFromArray(
    //        { static_cast<double>(sLeapHand.thumb.distal.next_joint.x)
    //        , static_cast<double>(sLeapHand.thumb.distal.next_joint.y)
    //        , static_cast<double>(sLeapHand.thumb.distal.next_joint.z) }),
    //    x_dPinchThresholdInMilliMeters);

    m_sOpenVRState.bMiddleClick = EvaluateDigitProximityForClick(
        m_pVectorFactory->V3DFromArray(
            { static_cast<double>(sLeapHand.middle.distal.next_joint.x)
            , static_cast<double>(sLeapHand.middle.distal.next_joint.y)
            , static_cast<double>(sLeapHand.middle.distal.next_joint.z) }),
        m_pVectorFactory->V3DFromArray(
            { static_cast<double>(sLeapHand.thumb.distal.next_joint.x)
            , static_cast<double>(sLeapHand.thumb.distal.next_joint.y)
            , static_cast<double>(sLeapHand.thumb.distal.next_joint.z) }),
        x_dPinchThresholdInMilliMeters);

    m_sOpenVRState.bRingClick = false;
    //m_sOpenVRState.bRingClick = EvaluateDigitProximityForClick(
    //    m_pVectorFactory->V3DFromArray(
    //        { static_cast<double>(sLeapHand.ring.distal.next_joint.x)
    //        , static_cast<double>(sLeapHand.ring.distal.next_joint.y)
    //        , static_cast<double>(sLeapHand.ring.distal.next_joint.z) }),
    //    m_pVectorFactory->V3DFromArray(
    //        { static_cast<double>(sLeapHand.thumb.distal.next_joint.x)
    //        , static_cast<double>(sLeapHand.thumb.distal.next_joint.y)
    //        , static_cast<double>(sLeapHand.thumb.distal.next_joint.z) }),
    //    x_dPinchThresholdInMilliMeters);

    m_sOpenVRState.bPinkyClick = EvaluateDigitProximityForClick(
        m_pVectorFactory->V3DFromArray(
            { static_cast<double>(sLeapHand.pinky.distal.next_joint.x)
            , static_cast<double>(sLeapHand.pinky.distal.next_joint.y)
            , static_cast<double>(sLeapHand.pinky.distal.next_joint.z) }),
        m_pVectorFactory->V3DFromArray(
            { static_cast<double>(sLeapHand.thumb.distal.next_joint.x)
            , static_cast<double>(sLeapHand.thumb.distal.next_joint.y)
            , static_cast<double>(sLeapHand.thumb.distal.next_joint.z) }),
        x_dPinchThresholdInMilliMeters);

    m_pSkeletalAdapter->FromLeapMotionHandToOVRBonePoseArray(
        sPose,
        m4dFromLeapSensorToHMDRelativeSpace,
        m4dFromHMDToWorldSpace,
        sLeapHand,
        m_sOpenVRState.asOpenVRSkeletalFrameData,
        m_eHand);

    m_sOpenVRState.bNewSkeletalFrameData = true;
}

//TODO: move this to a gesture recognizer
bool northstar::driver::CController::EvaluateDigitProximityForClick(const Vector3d& v3dDigitA, const Vector3d& v3dDigitB, const double& dPinchThreshold) {
    return (v3dDigitA - v3dDigitB).norm() <= dPinchThreshold;
}

void northstar::driver::CController::RunFrame() {
    if (m_sOpenVRState.unObjectId != vr::k_unTrackedDeviceIndexInvalid) {
        m_pVRServerDriverHost->TrackedDevicePoseUpdated(m_sOpenVRState.unObjectId, GetPose(), sizeof(vr::DriverPose_t));
        EmitAndClearInputStateEvents();
    }
}

void northstar::driver::CController::EmitAndClearInputStateEvents() {
    m_pVRDriverInput->UpdateScalarComponent(
        m_sOpenVRState.unTriggerValueComponent, 
        m_sOpenVRState.fTriggerValue,
        0);

    // TODO: Figure out better bindings
    //m_pVRDriverInput->UpdateBooleanComponent(
    //    m_sOpenVRState.unAClickComponent, 
    //    m_sOpenVRState.bMiddleClick,
    //    0);

    m_pVRDriverInput->UpdateBooleanComponent(
        m_sOpenVRState.unBClickComponent, 
        m_sOpenVRState.bMiddleClick,
        0);

    m_pVRDriverInput->UpdateBooleanComponent(
        m_sOpenVRState.unSystemClickComponent, 
        m_sOpenVRState.bPinkyClick,
        0);

    if (!m_sOpenVRState.bNewSkeletalFrameData && m_sOpenVRState.bSkeletalFrameDataInitialPoseSubmitted)
        return;

    for (const auto& eSkeletalMotionRange : x_aeSkeletalMotionRanges) {
        auto eResult = m_pVRDriverInput->UpdateSkeletonComponent(
            m_sOpenVRState.unSkeletalComponent,
            eSkeletalMotionRange,
            m_sOpenVRState.asOpenVRSkeletalFrameData.data(),
            static_cast<uint32_t>(m_sOpenVRState.asOpenVRSkeletalFrameData.size()));

        if (eResult != vr::EVRInputError::VRInputError_None) {
            m_pLogger->Log("Error updating skeletal input component state!");
        }
    }

    m_sOpenVRState.bSkeletalFrameDataInitialPoseSubmitted = true;
    m_sOpenVRState.bNewSkeletalFrameData = false;
}
