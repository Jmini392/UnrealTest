#include "Characters/SKCharacter.h"
#include "GAS/SKAttributeSet.h"
#include "GAS/SKGA_LightAttack.h"
#include "SKSaveGame.h"
#include "UI/SKOptionsMenu.h"
#include "Blueprint/UserWidget.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	// Graybox weapon poses per combo step: start/end rotation of the weapon pivot (pitch, yaw, roll).
	// Pitch -90 points the weapon forward; +40 leans it back over the shoulder.
	struct FSwingPose { FRotator Start; FRotator End; };
	const FSwingPose SwingPoses[] =
	{
		{ FRotator(40.f, 0.f, 0.f),   FRotator(-110.f, 0.f, 0.f) },    // overhead
		{ FRotator(-90.f, -80.f, 0.f), FRotator(-90.f, 80.f, 0.f) },    // horizontal sweep
		{ FRotator(40.f, 20.f, 0.f),  FRotator(-110.f, -20.f, 0.f) },  // overhead, mirrored lean
		{ FRotator(70.f, 0.f, 0.f),   FRotator(-130.f, 0.f, 0.f) },    // finisher, bigger arc
	};
	const FRotator WeaponRestRotation(20.f, 0.f, 0.f);
}

ASKCharacter::ASKCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.f);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->bOrientRotationToMovement = true;
	Movement->RotationRate = FRotator(0.f, 720.f, 0.f);
	Movement->MaxWalkSpeed = 600.f;
	Movement->BrakingDecelerationWalking = 2000.f;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 600.f;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));

	BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
	BodyMesh->SetupAttachment(RootComponent);
	BodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BodyMesh->SetRelativeScale3D(FVector(0.7f, 0.7f, 1.8f));
	if (Cylinder.Succeeded())
	{
		BodyMesh->SetStaticMesh(Cylinder.Object);
	}

	WeaponPivot = CreateDefaultSubobject<USceneComponent>(TEXT("WeaponPivot"));
	WeaponPivot->SetupAttachment(RootComponent);
	WeaponPivot->SetRelativeLocation(FVector(20.f, 55.f, 20.f));
	WeaponPivot->SetRelativeRotation(WeaponRestRotation);

	WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetupAttachment(WeaponPivot);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMesh->SetRelativeLocation(FVector(0.f, 0.f, 60.f));
	WeaponMesh->SetRelativeScale3D(FVector(0.15f, 0.15f, 1.2f));
	if (Cube.Succeeded())
	{
		WeaponMesh->SetStaticMesh(Cube.Object);
	}

	AbilitySystem = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystem"));
	Attributes = CreateDefaultSubobject<USKAttributeSet>(TEXT("Attributes"));

	LightAttackAbility = USKGA_LightAttack::StaticClass();
	OptionsMenuClass = USKOptionsMenu::StaticClass();
}

void ASKCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	// Options menu lives on the local player's screen for as long as Leo is possessed.
	if (!OptionsMenu && OptionsMenuClass)
	{
		if (APlayerController* PC = Cast<APlayerController>(NewController))
		{
			OptionsMenu = CreateWidget<USKOptionsMenu>(PC, OptionsMenuClass);
			if (OptionsMenu)
			{
				OptionsMenu->AddToViewport(100);
			}
		}
	}
}

void ASKCharacter::WriteSaveData(USKSaveGame& Out) const
{
	Out.PlayerTransform = GetActorTransform();
	Out.ControlRotation = Controller ? Controller->GetControlRotation() : GetActorRotation();
	Out.Health = Attributes->GetHealth();
	Out.UltGauge = Attributes->GetUltGauge();
}

void ASKCharacter::ApplySaveData(const USKSaveGame& In)
{
	SetActorTransform(In.PlayerTransform, false, nullptr, ETeleportType::TeleportPhysics);
	GetCharacterMovement()->StopMovementImmediately();
	if (Controller)
	{
		Controller->SetControlRotation(In.ControlRotation);
	}
	Attributes->SetHealth(In.Health);
	Attributes->SetUltGauge(In.UltGauge);
}

void ASKCharacter::BeginPlay()
{
	Super::BeginPlay();

	AbilitySystem->InitAbilityActorInfo(this, this);

	if (LightAttackAbility)
	{
		AbilitySystem->GiveAbility(FGameplayAbilitySpec(LightAttackAbility, 1, SKAbilityInput::LightAttack, this));
	}
}

void ASKCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	FTSTicker::GetCoreTicker().RemoveTicker(HitStopHandle);
	UGameplayStatics::SetGlobalTimeDilation(this, 1.f);

	Super::EndPlay(EndPlayReason);
}

void ASKCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bSwinging)
	{
		SwingElapsed += DeltaSeconds;
		const FSwingPose& Pose = SwingPoses[FMath::Clamp(SwingIndex, 0, UE_ARRAY_COUNT(SwingPoses) - 1)];
		const float T = FMath::Clamp(SwingElapsed / FMath::Max(SwingDuration, KINDA_SMALL_NUMBER), 0.f, 1.f);
		// Ease-in: the weapon accelerates into the impact.
		const float Eased = T * T;
		WeaponPivot->SetRelativeRotation(FQuat::Slerp(Pose.Start.Quaternion(), Pose.End.Quaternion(), Eased));

		// Hold the end pose briefly, then return to rest.
		if (SwingElapsed > SwingDuration + 0.2f)
		{
			bSwinging = false;
		}
	}
	else
	{
		WeaponPivot->SetRelativeRotation(FMath::RInterpTo(WeaponPivot->GetRelativeRotation(), WeaponRestRotation, DeltaSeconds, 12.f));
	}
}

void ASKCharacter::PlayWeaponSwing(int32 ComboIndex, float Duration)
{
	SwingIndex = ComboIndex;
	SwingDuration = Duration;
	SwingElapsed = 0.f;
	bSwinging = true;
}

void ASKCharacter::FaceInputOrCamera()
{
	if (!HeldMoveDirection.IsNearlyZero())
	{
		SetActorRotation(HeldMoveDirection.Rotation());
	}
}

void ASKCharacter::RequestHitStop(float Duration)
{
	if (Duration <= 0.f)
	{
		return;
	}

	UGameplayStatics::SetGlobalTimeDilation(this, 0.02f);

	// Core ticker runs on real time, so the freeze ends on schedule even though game time is nearly stopped.
	FTSTicker::GetCoreTicker().RemoveTicker(HitStopHandle);
	TWeakObjectPtr<ASKCharacter> WeakThis(this);
	HitStopHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([WeakThis](float)
	{
		if (WeakThis.IsValid())
		{
			UGameplayStatics::SetGlobalTimeDilation(WeakThis.Get(), 1.f);
		}
		return false;
	}), Duration);
}

void ASKCharacter::CreateDefaultInput()
{
	DefaultMappingContext = NewObject<UInputMappingContext>(this);

	auto MakeAction = [this](const TCHAR* Name, EInputActionValueType Type)
	{
		UInputAction* Action = NewObject<UInputAction>(this, Name);
		Action->ValueType = Type;
		return Action;
	};

	MoveAction = MakeAction(TEXT("IA_Move"), EInputActionValueType::Axis2D);
	LookAction = MakeAction(TEXT("IA_Look"), EInputActionValueType::Axis2D);
	LightAttackAction = MakeAction(TEXT("IA_LightAttack"), EInputActionValueType::Boolean);
	HeavyAttackAction = MakeAction(TEXT("IA_HeavyAttack"), EInputActionValueType::Boolean);
	DodgeAction = MakeAction(TEXT("IA_Dodge"), EInputActionValueType::Boolean);
	ToggleMenuAction = MakeAction(TEXT("IA_ToggleMenu"), EInputActionValueType::Boolean);
	// The menu pauses the game, and the same key has to close it again.
	ToggleMenuAction->bTriggerWhenPaused = true;

	// Keys give a single float, so W/S need a swizzle onto the Y axis; S and A are negated.
	auto MapAxisKey = [this](FKey Key, bool bNegate, bool bSwizzle)
	{
		FEnhancedActionKeyMapping& Mapping = DefaultMappingContext->MapKey(MoveAction, Key);
		if (bSwizzle)
		{
			UInputModifierSwizzleAxis* Swizzle = NewObject<UInputModifierSwizzleAxis>(this);
			Swizzle->Order = EInputAxisSwizzle::YXZ;
			Mapping.Modifiers.Add(Swizzle);
		}
		if (bNegate)
		{
			Mapping.Modifiers.Add(NewObject<UInputModifierNegate>(this));
		}
	};

	MapAxisKey(EKeys::W, false, true);
	MapAxisKey(EKeys::S, true, true);
	MapAxisKey(EKeys::D, false, false);
	MapAxisKey(EKeys::A, true, false);
	DefaultMappingContext->MapKey(MoveAction, EKeys::Gamepad_Left2D);

	DefaultMappingContext->MapKey(LookAction, EKeys::Mouse2D);
	DefaultMappingContext->MapKey(LookAction, EKeys::Gamepad_Right2D);

	DefaultMappingContext->MapKey(LightAttackAction, EKeys::LeftMouseButton);
	DefaultMappingContext->MapKey(LightAttackAction, EKeys::Gamepad_FaceButton_Left);
	DefaultMappingContext->MapKey(HeavyAttackAction, EKeys::RightMouseButton);
	DefaultMappingContext->MapKey(HeavyAttackAction, EKeys::Gamepad_FaceButton_Top);
	DefaultMappingContext->MapKey(DodgeAction, EKeys::SpaceBar);
	DefaultMappingContext->MapKey(DodgeAction, EKeys::Gamepad_FaceButton_Bottom);

	// Esc ends Play In Editor, so the menu is on 1 for now (Start button on a pad).
	DefaultMappingContext->MapKey(ToggleMenuAction, EKeys::One);
	DefaultMappingContext->MapKey(ToggleMenuAction, EKeys::Gamepad_Special_Right);
}

void ASKCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	CreateDefaultInput();

	if (const APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (ULocalPlayer* LP = PC->GetLocalPlayer())
		{
			if (auto* Subsystem = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
			{
				Subsystem->AddMappingContext(DefaultMappingContext, 0);
			}
		}
	}

	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EIC->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ASKCharacter::Move);
		EIC->BindAction(MoveAction, ETriggerEvent::Completed, this, &ASKCharacter::MoveReleased);
		EIC->BindAction(LookAction, ETriggerEvent::Triggered, this, &ASKCharacter::Look);
		EIC->BindAction(LightAttackAction, ETriggerEvent::Started, this, &ASKCharacter::OnLightAttackPressed);
		EIC->BindAction(ToggleMenuAction, ETriggerEvent::Started, this, &ASKCharacter::OnToggleMenuPressed);
		// Heavy attack and dodge are bound in the next steps.
	}
}

void ASKCharacter::Move(const FInputActionValue& Value)
{
	const FVector2D Input = Value.Get<FVector2D>();
	if (!Controller)
	{
		return;
	}

	const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);
	const FVector Forward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector Right = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	HeldMoveDirection = (Forward * Input.Y + Right * Input.X).GetSafeNormal();

	if (!bMovementLocked)
	{
		AddMovementInput(Forward, Input.Y);
		AddMovementInput(Right, Input.X);
	}
}

void ASKCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D Input = Value.Get<FVector2D>();
	AddControllerYawInput(Input.X);
	AddControllerPitchInput(-Input.Y);
}

void ASKCharacter::OnToggleMenuPressed()
{
	if (OptionsMenu)
	{
		OptionsMenu->ToggleMenu();
	}
}

void ASKCharacter::OnLightAttackPressed()
{
	// Starts the ability, or buffers the next combo hit if it is already running.
	AbilitySystem->AbilityLocalInputPressed(SKAbilityInput::LightAttack);
}
