#include <WPILib.h>
#include <CANTalon.h>

#include <algorithm>

#include "general/XBox.h"
#include "general/Talons.h"
#include "general/ToggleButton.h"

#include "subsystems/drive/BreakerVision.h"
#include "subsystems/drive/Drive.h"
#include "subsystems/Wings.h"


class Chadwick: public IterativeRobot {

private:
	Joystick xbox;

	std::shared_ptr<NetworkTable> subsystems;
	std::shared_ptr<NetworkTable> pixy;


	PIDController gearPlacer;
	float gearPlacerIZone;

	BreakerVision aiming;
	Drive drive;
	Drive::AutonomousMode autonomousMode;

	Wings wings;

	CANTalon slurper;
	ToggleButton slurperButton;

	CANTalon winch;
	float winchEffort;
	ToggleButton winchButton;

public:
	Chadwick():
		xbox(0),

		gearPlacer(0.025,0.0002,0,&aiming,&drive),
		gearPlacerIZone(100),

		aiming(),
		drive(&gearPlacer.m_totalError),
		autonomousMode(Drive::kGear3),

		wings(),

		slurper(Talons::INTAKE),
		slurperButton(XBox::BACK,true),

		winch(Talons::WINCH),
		winchEffort(0),
		winchButton(XBox::RB)

	{
		//----------Initializations---------------//

		pixy = NetworkTable::GetTable("GearPixy");
		subsystems = NetworkTable::GetTable("Subsystems");

	}//Robot Constructor

private:

	void RobotInit(){
//		CameraServer::GetInstance()->StartAutomaticCapture();
	}

	void Init(){
//		printf("Vision init\n");
		aiming.InitTable(pixy);
//		printf("Vision init done\n");

		drive.Init(subsystems->GetSubTable("Drive"), pixy);
		wings.Init(subsystems->GetSubTable("Wings"));

		//Begin the feed from aiming to drive
		gearPlacer.Enable();
		//Different than previous PID systems where the setpoint
		//changes in a stable environment, here, the setpoint is
		//always 0, and BreakerVision returns the error off of that
		//setpoint
		gearPlacer.SetSetpoint(0);
		gearPlacer.InitTable(pixy->GetSubTable("PID"));


	}

	void AutonomousInit(){
		//Called once at the start of each Auto period
		printf("Autonomous Initialized\n");
		Init();
		drive.AutonomousInit();
//		autonomousMode = Drive::AutonomousMode::kGear1;
		autonomousMode = (Drive::AutonomousMode) (int) subsystems->GetSubTable("Drive")->GetNumber("AutonomousMode",0);

		printf("End of Autonomous init\n");

	}

	void AutonomousPeriodic(){
		//Called PERIODICALLY during the Auto period

		aiming.Update();
		drive.Autonomous(autonomousMode);

		//Update PID loop
		//PIDController gearPlacer will automatically read error from aiming,
		//calculate controlEffort, and output that value to the drive system.
//		gearPlacer.SetPID(SmartDashboard::GetNumber("DB/Slider 0",0),SmartDashboard::GetNumber("DB/Slider 1",0),0);


		if (gearPlacer.GetError() >= gearPlacerIZone){
			gearPlacer.m_totalError = 0;
		}
	}

	void TeleopInit(){
		//Called once at the start of each operator period
		printf("TELEOP Initialized\n");
		Init();


		printf("End of Teleop Init\n");

	}//Teleop Init
	void DisabledInit(){
		printf("Disabled TELEOP\n");
	}

	void TeleopPeriodic(){
		//Called PERIODICALLY during the operator period

		//-----------Drive System------------//
		aiming.Update();
		drive.Update(xbox);

		//Update PID loop
		//PIDController gearPlacer will automatically read error from aiming,
		//calculate controlEffort, and output that value to the drive system.
//		gearPlacer.SetPID(SmartDashboard::GetNumber("DB/Slider 0",0),SmartDashboard::GetNumber("DB/Slider 1",0),0);

		if (xbox.GetRawButton(XBox::BACK)){
			gearPlacer.m_totalError = 0;
		}
		if (gearPlacer.GetError() >= gearPlacerIZone){
			gearPlacer.m_totalError = 0;
		}

		//-----------Gear Wings-----------//
		wings.Update(xbox);

		//-------------Winch----------------//

		bool winchEnabled = false;
		if (winchEnabled){
			static Deadband ryDeadband(0.1);
			float ryVal = 0;
			float winchOutput = 0;
			if (winchButton.State()){
				winchOutput = winchEffort;
			} else {
				ryVal = ryDeadband.OutputFor(xbox.GetRawAxis(XBox::RY));
				winchOutput = ryVal;
			}

			if (winchOutput< 0) winchOutput *= -1;
			winch.Set(winchOutput);
			SmartDashboard::PutNumber("Winch Output%",winchOutput);
			SmartDashboard::PutNumber("Winch Current Draw",winch.GetOutputCurrent());

			//Toggle Winch Autonomous
			winchButton.Update(xbox);
			if (winchButton.State() && !winchButton.PrevState()) winchEffort = ryVal;
		}

		bool slurperEnabled = false;
		if (slurperEnabled){
			static Deadband rxDeadband(0.1);
			float rxVal = 0;
			rxVal = rxDeadband.OutputFor(xbox.GetRawAxis(XBox::RX));

			//XBox::BACK
			if (slurperButton.State())
				slurper.Set(rxVal);

			SmartDashboard::PutNumber("Slurper Output%: ",rxVal);
			SmartDashboard::PutNumber("Slurper Current Draw",slurper.GetOutputCurrent());
			slurperButton.Update(xbox);
		}

		//-----------------------------------//
	}//teleop Periodic

	void TestPeriodic(){
	}
};

START_ROBOT_CLASS(Chadwick)
