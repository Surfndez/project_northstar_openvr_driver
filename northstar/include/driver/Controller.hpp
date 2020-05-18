#include <openvr_driver.h>
#include <memory>

#if defined( _WINDOWS )
#include <windows.h>
#endif

#include "utility/ILogger.hpp"
#include "driver/Types.hpp"
#include "driver/Settings.hpp"
#include "driver/ISensorFrameCoordinator.hpp"
#include "math/IWorldAdapter.hpp"
#include "math/IVectorFactory.hpp"
#include "math/ISkeletalAdapter.hpp"
#include "openvr/IVRProperties.hpp"
#include "math/Types.hpp"

namespace northstar {
    namespace driver {
        class CController : public vr::ITrackedDeviceServerDriver
        {
        public:
            CController(
                std::shared_ptr<northstar::driver::ISensorFrameCoordinator> pSensorFrameCoordinator,
                std::shared_ptr<northstar::math::IWorldAdapter> pWorldAdapter,
                std::shared_ptr<northstar::math::IVectorFactory> pVectorFactory,
                std::shared_ptr<northstar::math::ISkeletalAdapter> pSkeletalAdapter,
                std::shared_ptr<northstar::utility::ILogger> pLogger, 
                std::shared_ptr<northstar::openvr::IVRProperties> pVRProperties,
                vr::IVRSettings* pVRSettings,
                vr::IVRServerDriverHost* pVRServerDriverHost,
                vr::IVRDriverInput* pVRDriverInput,
                const northstar::driver::types::EHand& eHand);

            virtual vr::EVRInitError Activate(vr::TrackedDeviceIndex_t unObjectId) override final;
            virtual void Deactivate() override final;
            void* GetComponent(const char* pchComponentNameAndVersion) override final;
            const std::string_view& GetSerialNumber() const;
            virtual void EnterStandby() override final;
            virtual void DebugRequest(const char* pchRequest, char* pchResponseBuffer, uint32_t unResponseBufferSize);
            virtual vr::DriverPose_t GetPose() override;
            void RunFrame();
        private:
            static constexpr double x_dPinchThresholdInMilliMeters = 30; // how close do digits have to be to be considered "pinched"
            static constexpr std::string_view x_svModelNumber = "NorthStarHumanHand";
            static constexpr std::string_view x_svSerialNumberLeft = "LeftHand";
            static constexpr std::string_view x_svSerialNumberRight = "RightHand";
            static constexpr std::string_view x_svControllerType = "northstar_hand";
            static constexpr std::string_view x_svRenderModelNameLeft = "vr_controller_vive_1_5";
            static constexpr std::string_view x_svRenderModelNameRight = "vr_controller_vive_1_5";
            static constexpr vr::EVRSkeletalTrackingLevel x_eSkeletalTrackingLevel = vr::EVRSkeletalTrackingLevel::VRSkeletalTracking_Full;
            static constexpr vr::VRBoneTransform_t* x_vrGripLimitTransforms = nullptr;
            static constexpr uint32_t x_vrGripLimitTransformCount = 0;
            static constexpr std::array<vr::EVRSkeletalMotionRange, 2> x_aeSkeletalMotionRanges = {
                vr::EVRSkeletalMotionRange::VRSkeletalMotionRange_WithController,
                vr::EVRSkeletalMotionRange::VRSkeletalMotionRange_WithoutController};

            struct SLeapMotionConfiguration {
                northstar::math::types::Vector3d v3dPosition;
                northstar::math::types::Quaterniond qdOrientation;
            };

            struct SConfiguration {
                SLeapMotionConfiguration sLeapMotionConfiguration;
                bool bUseDebugTrackerConfig;
                bool bUseDebugBasePose;
                bool bUseRenderModel;
            };

            // TODO: move input state into gesture recognizer
            // TODO: clean up input (better as pull triggers than buttons)
            struct SOpenVRState {
                vr::TrackedDeviceIndex_t unObjectId;
                vr::PropertyContainerHandle_t ulPropertyContainer;
                vr::VRInputComponentHandle_t unTriggerValueComponent;
                vr::VRInputComponentHandle_t unAClickComponent;
                vr::VRInputComponentHandle_t unBClickComponent;
                vr::VRInputComponentHandle_t unSystemClickComponent;
                vr::VRInputComponentHandle_t unSkeletalComponent;
                std::array<vr::VRBoneTransform_t, northstar::driver::settings::values::driverConfiguration::k_unBoneCount> asOpenVRSkeletalFrameData;
                float fTriggerValue;
                bool bPointerClick;
                bool bMiddleClick;
                bool bRingClick;
                bool bPinkyClick;
                bool bNewSkeletalFrameData;
                bool bSkeletalFrameDataInitialPoseSubmitted;
            };

            void ClearOpenVRState();
            void LoadConfiguration();
            void SetOpenVRProperties();
            void CreateOpenVRInputComponents();
            void UpdatePendingInputState(
                const vr::DriverPose_t& sPose,
                const northstar::math::types::AffineMatrix4d& m4dFromLeapSensorToHMDRelativeSpace,
                const northstar::math::types::AffineMatrix4d& m4dFromHMDToWorldSpace,
                const LEAP_HAND& sLeapHand);

            bool EvaluateDigitProximityForClick(
                const northstar::math::types::Vector3d& v3dDigitA, 
                const northstar::math::types::Vector3d& v3dDigitB, 
                const double& dPinchThreshold);

            void EmitAndClearInputStateEvents();

            SConfiguration m_sConfiguration;
            SOpenVRState m_sOpenVRState;

            std::shared_ptr<northstar::openvr::IVRProperties> m_pVRProperties;
            std::shared_ptr<northstar::math::IWorldAdapter> m_pWorldAdapter;
            std::shared_ptr<northstar::math::IVectorFactory> m_pVectorFactory;
            std::shared_ptr<northstar::math::ISkeletalAdapter> m_pSkeletalAdapter;
            vr::IVRSettings* m_pVRSettings;
            vr::IVRServerDriverHost* m_pVRServerDriverHost;
            vr::IVRDriverInput* m_pVRDriverInput;
            std::shared_ptr<northstar::driver::ISensorFrameCoordinator> m_pSensorFrameCoordinator;
            std::shared_ptr<northstar::utility::ILogger> m_pLogger;
            northstar::driver::types::EHand m_eHand;
        };
    }
}
