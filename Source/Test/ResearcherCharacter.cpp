#include "ResearcherCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "UObject/ConstructorHelpers.h"
#include "ResearcherAnimInstance.h"
#include "ResearcherDrone.h"
#include "ResearcherSkillFx.h"
#include "ResearcherLightningFx.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "EngineUtils.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/OverlapResult.h"

AResearcherCharacter::AResearcherCharacter()
{
	// 펄스 연출용 나이아가라 시스템 (없으면 메시 연출로 대체)
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> PulseSystem(TEXT("/Game/VFX/NS_PulseBurst.NS_PulseBurst"));
	if (PulseSystem.Succeeded())
	{
		PulseEffect = PulseSystem.Object;
	}

	GetCapsuleComponent()->InitCapsuleSize(35.f, 90.f);

	// 카메라는 컨트롤러 회전을 따르고 캐릭터는 이동 방향으로 몸을 돌린다
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->bOrientRotationToMovement = true;
	Movement->RotationRate = FRotator(0.f, 540.f, 0.f);
	Movement->MaxWalkSpeed = WalkSpeed;
	Movement->BrakingDecelerationWalking = 1000.f;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 350.f;
	CameraBoom->SocketOffset = FVector(0.f, 0.f, 60.f);
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// 캡슐 바닥에 발이 오도록 메시 위치를 내리고, 앞(+X)을 보게 돌린다. 모델이 다른 쪽을 보면 BP에서 Yaw를 조정
	USkeletalMeshComponent* MeshComp = GetMesh();
	MeshComp->SetRelativeLocation(FVector(0.f, 0.f, -90.f));
	MeshComp->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> MeshFinder(TEXT("/Game/Characters/Resercher/Researcher.Researcher"));
	if (MeshFinder.Succeeded())
	{
		MeshComp->SetSkeletalMesh(MeshFinder.Object);
	}

	static ConstructorHelpers::FClassFinder<UAnimInstance> AnimFinder(TEXT("/Game/Characters/Resercher/ABP_Researcher.ABP_Researcher_C"));
	if (AnimFinder.Succeeded())
	{
		MeshComp->SetAnimationMode(EAnimationMode::AnimationBlueprint);
		MeshComp->SetAnimInstanceClass(AnimFinder.Class);
	}
	else
	{
		MeshComp->SetAnimInstanceClass(UResearcherAnimInstance::StaticClass());
	}
}

void AResearcherCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	BuildDefaultInput();

	if (const APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}

	if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AResearcherCharacter::Move);
		EnhancedInput->BindAction(LookAction, ETriggerEvent::Triggered, this, &AResearcherCharacter::Look);
		EnhancedInput->BindAction(SprintAction, ETriggerEvent::Started, this, &AResearcherCharacter::StartSprint);
		EnhancedInput->BindAction(SprintAction, ETriggerEvent::Completed, this, &AResearcherCharacter::StopSprint);
		EnhancedInput->BindAction(SprintAction, ETriggerEvent::Canceled, this, &AResearcherCharacter::StopSprint);
		EnhancedInput->BindAction(PulseAction, ETriggerEvent::Started, this, &AResearcherCharacter::EmitPulse);
		EnhancedInput->BindAction(DroneViewAction, ETriggerEvent::Started, this, &AResearcherCharacter::ToggleDroneView);
		EnhancedInput->BindAction(ReleaseDroneAction, ETriggerEvent::Started, this, &AResearcherCharacter::ReleaseDrone);
		EnhancedInput->BindAction(RiseAction, ETriggerEvent::Started, this, &AResearcherCharacter::OnRisePressed);
		EnhancedInput->BindAction(RiseAction, ETriggerEvent::Completed, this, &AResearcherCharacter::OnRiseReleased);
		EnhancedInput->BindAction(RiseAction, ETriggerEvent::Canceled, this, &AResearcherCharacter::OnRiseReleased);
	}
}

void AResearcherCharacter::BuildDefaultInput()
{
	if (!MoveAction)
	{
		MoveAction = NewObject<UInputAction>(this, TEXT("IA_Move_Default"));
		MoveAction->ValueType = EInputActionValueType::Axis2D;
	}
	if (!LookAction)
	{
		LookAction = NewObject<UInputAction>(this, TEXT("IA_Look_Default"));
		LookAction->ValueType = EInputActionValueType::Axis2D;
	}
	if (!SprintAction)
	{
		SprintAction = NewObject<UInputAction>(this, TEXT("IA_Sprint_Default"));
		SprintAction->ValueType = EInputActionValueType::Boolean;
	}
	auto MakeButton = [this](TObjectPtr<UInputAction>& Action, const TCHAR* Name)
	{
		if (!Action)
		{
			Action = NewObject<UInputAction>(this, Name);
			Action->ValueType = EInputActionValueType::Boolean;
		}
	};
	MakeButton(PulseAction, TEXT("IA_Pulse_Default"));
	MakeButton(DroneViewAction, TEXT("IA_DroneView_Default"));
	MakeButton(ReleaseDroneAction, TEXT("IA_ReleaseDrone_Default"));
	MakeButton(RiseAction, TEXT("IA_Rise_Default"));
	if (DefaultMappingContext)
	{
		return;
	}

	DefaultMappingContext = NewObject<UInputMappingContext>(this, TEXT("IMC_Default_Runtime"));

	// 축 값은 (X = 좌우, Y = 앞뒤). W/S는 Swizzle로 값을 Y축으로 옮기고 S와 A는 부호를 뒤집는다
	FEnhancedActionKeyMapping& KeyW = DefaultMappingContext->MapKey(MoveAction, EKeys::W);
	KeyW.Modifiers.Add(NewObject<UInputModifierSwizzleAxis>(this));

	FEnhancedActionKeyMapping& KeyS = DefaultMappingContext->MapKey(MoveAction, EKeys::S);
	KeyS.Modifiers.Add(NewObject<UInputModifierSwizzleAxis>(this));
	KeyS.Modifiers.Add(NewObject<UInputModifierNegate>(this));

	FEnhancedActionKeyMapping& KeyA = DefaultMappingContext->MapKey(MoveAction, EKeys::A);
	KeyA.Modifiers.Add(NewObject<UInputModifierNegate>(this));

	DefaultMappingContext->MapKey(MoveAction, EKeys::D);

	DefaultMappingContext->MapKey(SprintAction, EKeys::LeftShift);
	DefaultMappingContext->MapKey(PulseAction, EKeys::R);
	DefaultMappingContext->MapKey(DroneViewAction, EKeys::Q);
	DefaultMappingContext->MapKey(ReleaseDroneAction, EKeys::E);
	DefaultMappingContext->MapKey(RiseAction, EKeys::SpaceBar);

	// 마우스 위로 = 위를 보기. 피치는 부호가 반대라 Y를 뒤집는다
	FEnhancedActionKeyMapping& Mouse = DefaultMappingContext->MapKey(LookAction, EKeys::Mouse2D);
	UInputModifierNegate* NegateY = NewObject<UInputModifierNegate>(this);
	NegateY->bX = false;
	NegateY->bY = true;
	NegateY->bZ = false;
	Mouse.Modifiers.Add(NegateY);
}

void AResearcherCharacter::Move(const FInputActionValue& Value)
{
	const FVector2D Input = Value.Get<FVector2D>();
	if (!Controller)
	{
		return;
	}

	if (IsDroneViewActive())
	{
		// 드론 비행: 앞뒤는 카메라 방향(위를 보면 위로), 좌우는 수평
		const FRotator ControlRotation = Controller->GetControlRotation();
		const FVector Forward = ControlRotation.Vector();
		const FVector Right = FRotationMatrix(FRotator(0.f, ControlRotation.Yaw, 0.f)).GetUnitAxis(EAxis::Y);
		Drone->AddFlightInput(Forward * Input.Y + Right * Input.X, 1.f);
		return;
	}

	// 카메라가 보는 방향(Yaw)을 기준으로 앞/오른쪽을 구한다
	const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);
	const FRotationMatrix RotationMatrix(YawRotation);
	AddMovementInput(RotationMatrix.GetUnitAxis(EAxis::X), Input.Y);
	AddMovementInput(RotationMatrix.GetUnitAxis(EAxis::Y), Input.X);
}

void AResearcherCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D Input = Value.Get<FVector2D>();
	AddControllerYawInput(Input.X);
	AddControllerPitchInput(Input.Y);
}

void AResearcherCharacter::StartSprint()
{
	bWantsToSprint = true;
	GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;
}

void AResearcherCharacter::StopSprint()
{
	bWantsToSprint = false;
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
}

void AResearcherCharacter::BeginPlay()
{
	Super::BeginPlay();

	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Drone = GetWorld()->SpawnActor<AResearcherDrone>(AResearcherDrone::StaticClass(), GetActorLocation() + FVector(0.f, 0.f, 170.f), GetActorRotation(), Params);
	if (Drone)
	{
		Drone->InitForCharacter(this);
	}
}

void AResearcherCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Drone)
	{
		Drone->Destroy();
		Drone = nullptr;
	}
	Super::EndPlay(EndPlayReason);
}

void AResearcherCharacter::ToggleDroneView()
{
	if (!Drone)
	{
		return;
	}
	if (bDroneView)
	{
		// 시야에서 나온다: 드론은 지금 자리에 고정된다
		Drone->SetParked(true, Drone->GetActorLocation());
	}
	bDroneView = !bDroneView;
	Drone->SetViewActive(bDroneView);

	// 달리던 중이었다면 멈춘다 (비행 중에는 걷기 속도가 쓰이지 않는다)
	if (bDroneView)
	{
		GetCharacterMovement()->StopMovementImmediately();
	}
	if (APlayerController* PC = Cast<APlayerController>(Controller))
	{
		PC->SetViewTargetWithBlend(bDroneView ? static_cast<AActor*>(Drone) : static_cast<AActor*>(this), 0.2f);
	}
}

void AResearcherCharacter::ReleaseDrone()
{
	if (!Drone)
	{
		return;
	}
	Drone->SetParked(false, FVector::ZeroVector);
	if (bDroneView)
	{
		bDroneView = false;
		Drone->SetViewActive(false);
		if (APlayerController* PC = Cast<APlayerController>(Controller))
		{
			PC->SetViewTargetWithBlend(this, 0.2f);
		}
	}
}

void AResearcherCharacter::OnRisePressed()
{
	if (IsDroneViewActive())
	{
		Drone->SetRise(true); // 드론 시야에서는 스페이스가 상승
	}
}

void AResearcherCharacter::OnRiseReleased()
{
	if (Drone)
	{
		Drone->SetRise(false);
	}
}

void AResearcherCharacter::EmitPulse()
{
	// 드론 시야 중에는 쓸 수 없다. 쿨다운 중에도 쓸 수 없다
	if (IsDroneViewActive() || !GetWorld())
	{
		return;
	}
	const double Now = GetWorld()->GetTimeSeconds();
	if (Now < NextPulseTime)
	{
		return;
	}
	NextPulseTime = Now + PulseCooldown;

	// 펄스는 발밑을 중심으로 퍼진다
	const FVector Origin = GetActorLocation() - FVector(0.f, 0.f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight());

	// 연출: 나이아가라 시스템이 있으면 그것을, 없으면 메시로 만든 대체 연출을 쓴다
	if (PulseEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), PulseEffect, Origin, FRotator::ZeroRotator, FVector::OneVector, true);
	}
	else
	{
		SpawnMeshPulseFx(Origin);
	}

	// 반경 안의 다른 캐릭터를 바깥으로 밀어낸다
	for (TActorIterator<ACharacter> It(GetWorld()); It; ++It)
	{
		ACharacter* Other = *It;
		if (Other == this)
		{
			continue;
		}
		FVector Away = Other->GetActorLocation() - Origin;
		if (Away.Size2D() > PulseRadius)
		{
			continue;
		}
		Away = Away.GetSafeNormal2D();
		if (Away.IsNearlyZero())
		{
			Away = GetActorForwardVector().GetSafeNormal2D();
		}
		Other->LaunchCharacter(Away * (PulsePushDistance / PulsePushDuration), true, false);
	}

	// 물리 시뮬레이션 중인 물체도 바깥으로 민다
	if (PulsePhysicsPush > 0.f)
	{
		TArray<FOverlapResult> Overlaps;
		FCollisionObjectQueryParams ObjectTypes(ECC_PhysicsBody);
		ObjectTypes.AddObjectTypesToQuery(ECC_WorldDynamic);
		FCollisionQueryParams Query(NAME_None, false, this);
		GetWorld()->OverlapMultiByObjectType(Overlaps, Origin, FQuat::Identity, ObjectTypes, FCollisionShape::MakeSphere(PulseRadius), Query);
		for (const FOverlapResult& Overlap : Overlaps)
		{
			if (UPrimitiveComponent* Component = Overlap.GetComponent())
			{
				if (Component->IsSimulatingPhysics())
				{
					Component->AddRadialImpulse(Origin, PulseRadius, PulsePhysicsPush, ERadialImpulseFalloff::RIF_Linear, true);
				}
			}
		}
	}

	OnPulseEmitted();
}

void AResearcherCharacter::SpawnMeshPulseFx(const FVector& Origin)
{
	// 연출 (정전기장 스타일): 중심의 푸른 섬광 -> 빠르게 퍼지는 전기 충격파 링 -> 바닥을 타고 뻗는 번개 -> 옅은 푸른 돔.
	// 1보다 큰 색 값은 포스트 프로세스 블룸으로 번쩍이게 한다
	const float Diameter = PulseRadius * 2.f / 100.f;
	UWorld* World = GetWorld();

	// 중심 섬광: 몸 높이에서 순식간에 터졌다 사라진다
	if (AResearcherSkillFx* Flash = World->SpawnActor<AResearcherSkillFx>(Origin + FVector(0.f, 0.f, 60.f), FRotator::ZeroRotator))
	{
		Flash->Play(EResearcherFxShape::Sphere, FLinearColor(2.f, 4.f, 8.f), 0.18f, FVector(0.4f), FVector(Diameter * 0.3f), 0.95f, 0.f);
	}

	// 충격파 링: 얇고 밝은 고리가 반경 끝까지 빠르게 퍼진다. 평면 반지름의 0.9 지점에 고리가 있으므로 평면은 그만큼 키운다
	constexpr float RingAt = 0.9f;
	if (AResearcherSkillFx* Shock = World->SpawnActor<AResearcherSkillFx>(Origin + FVector(0.f, 0.f, 12.f), FRotator::ZeroRotator))
	{
		Shock->SetRingStyle(RingAt, 0.1f, 5.f);
		Shock->Play(EResearcherFxShape::Ring, FLinearColor(0.6f, 1.4f, 3.f), 0.45f, FVector(Diameter / RingAt * 0.05f), FVector(Diameter / RingAt), 1.f, 0.f);
	}
	// 두 번째 고리: 더 굵고 푸르게, 조금 느리게 따라온다
	if (AResearcherSkillFx* Wake = World->SpawnActor<AResearcherSkillFx>(Origin + FVector(0.f, 0.f, 8.f), FRotator::ZeroRotator))
	{
		Wake->SetRingStyle(RingAt, 0.25f, 3.f);
		Wake->Play(EResearcherFxShape::Ring, FLinearColor(0.15f, 0.5f, 1.5f), 0.7f, FVector(Diameter / RingAt * 0.05f), FVector(Diameter / RingAt), 0.7f, 0.f);
	}

	// 바닥 전기장: 옅은 푸른 원반이 깔렸다가 사라진다
	if (AResearcherSkillFx* Field = World->SpawnActor<AResearcherSkillFx>(Origin + FVector(0.f, 0.f, 6.f), FRotator::ZeroRotator))
	{
		Field->Play(EResearcherFxShape::Disc, FLinearColor(0.1f, 0.4f, 1.2f), 0.7f, FVector(0.3f, 0.3f, 0.02f), FVector(Diameter, Diameter, 0.02f), 0.4f, 0.f);
	}

	// 옅은 푸른 돔: 반경 끝까지 함께 부푼다
	if (AResearcherSkillFx* Dome = World->SpawnActor<AResearcherSkillFx>(Origin, FRotator::ZeroRotator))
	{
		Dome->Play(EResearcherFxShape::Sphere, FLinearColor(0.2f, 0.55f, 1.5f), 0.35f, FVector(Diameter * 0.15f), FVector(Diameter), 0.3f, 0.f);
	}

	// 번개 줄기: 중심에서 바닥을 따라 바깥으로 뻗으며 번쩍인다
	if (AResearcherLightningFx* Bolts = World->SpawnActor<AResearcherLightningFx>(Origin, FRotator::ZeroRotator))
	{
		Bolts->Play(PulseRadius, 0.6f, FLinearColor(1.f, 2.2f, 5.f), 9);
	}
}
