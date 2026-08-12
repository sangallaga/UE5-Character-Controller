# UE5 Character Controller — Learning Log

**Project:** Character controller demo (portfolio piece targeting games systems engineer roles)
**Engine:** Unreal Engine 5 · **IDE:** JetBrains Rider · **Input:** Enhanced Input

**Learning contract:** Direct answers on tooling and syntax; Socratic nudging reserved for core architectural concepts worth reasoning through independently.

---

## 1. Blueprint or C++?

**Q: How do I approach this project? Do I start with a blueprint?**

The premise is a false dichotomy — Blueprint and C++ aren't competing choices, they're layers. The idiomatic pattern, and the one recruiters look for:

> **Write the system in C++, then make a Blueprint that inherits from your C++ class to hold data** — which mesh, which input assets, what the walk speed is.

Engineer builds the machine; the Blueprint is the dashboard. If you catch yourself wiring movement logic into Blueprint nodes, you've drifted from what the demo is meant to prove.

### Decisions at project creation

- **Blueprint project vs C++ project** — a C++ project gets Rider attached to a real solution from minute one. Converting a BP project after the fact works, but means your first lesson is a workaround.
- **Third Person template vs Blank** — the template ships a fully working character controller. Decide deliberately whether it's a crutch (90% Epic's code with your name on it) or a reference implementation you read and then rebuild from Blank. Very different portfolio pieces. Test: what could you actually say about that code when asked to walk through it?

### Landmine

Modern Unreal uses **Enhanced Input** — Input Actions and Input Mapping Contexts as Content Browser assets. Many tutorials still teach the legacy system where you type axis names into Project Settings. If you hit one of those, it's dated. Useful smell test for filtering material.

---

## 2. C++ background check

Prior experience: ~1 year across an intro C++ course (headers, syntax, encapsulation, pointers, classes, inheritance) and a data structures course.

> *"Python comes and goes, but when it comes to C++ you either keep it or lose it."*

That's about muscle memory, not concepts. Syntax returns within a week of active use; the mental models underneath (what a pointer actually is, who owns what, how a vtable makes `virtual` work) are the expensive part, already paid for.

The list is well-targeted: Unreal's gameplay framework **is** an inheritance tree — `ACharacter` → `APawn` → `AActor` — and daily work is subclassing and overriding virtuals. Data structures matters less directly, but it's why `TArray` and `TMap` won't cause panic.

### Brace for this

**Unreal C++ is a dialect, not the C++ you learned.** Reaching for `std::vector`, `std::string`, or `new`/`delete` on engine objects is wrong in ways that aren't immediately obvious. There's a parallel standard library and a memory model you don't control directly.

---

## 3. Building a testbed

**Q: I have a blank project with no boilerplate. How do I create a space to test these concepts?**

The sandbox *is* the experiment:

1. Editor → **File → New C++ Class → Actor**. Name it something disposable: `ATestbedActor`.
2. Drag it into the level.
3. Add `UE_LOG(LogTemp, Warning, TEXT("..."))` inside `BeginPlay()`.
4. **Window → Output Log**, hit Play.

Once that message appears, you have a working feedback cycle and can test almost anything.

**Sanity check:** does your project have a `Source` folder next to `Content`? If not, you made a Blueprint-only blank project and Rider is attached to nothing useful.

**Two time-savers:**
- `TEXT()` around string literals isn't decoration — worth ten seconds of curiosity about why.
- **Live Coding** (compile button, bottom-right of editor) works for changing function bodies but gets flaky when adding new `UPROPERTY` members. When something refuses to appear: close editor, build from Rider, reopen. Not you doing it wrong.

---

## 4. Building from Rider

**Q: What do you mean "build from Rider"? Where does this happen?**

**Toolbar (top-right):** a dropdown showing the build target — something like `MyProjectEditor | Development Editor | Win64`. That target produces the DLL the Unreal editor loads. Next to it, a hammer icon = Build.

**Menu:** Build → Build Solution. **Shortcut:** `Ctrl+F9` (Rider default) or `Ctrl+Shift+B` (Visual Studio keymap).

**What actually happens:** Rider hands off to **Unreal Build Tool**, which runs **Unreal Header Tool** first (generating `.generated.h` files), then compiles and links. Output lands in:

```
YourProject/Binaries/Win64/UnrealEditor-YourProject.dll
```

On Mac the target reads `Mac` and you get a `.dylib`; everything else is the same.

### The part that trips everyone up

**Close the Unreal editor before building this way.** A running editor holds that DLL open and the linker can't overwrite a locked file:

```
LNK1168: cannot open ... .dll for writing
```

That's not a code problem — the editor is still running.

**Full cycle for "I added a UPROPERTY and it won't show up":**
close editor → build in Rider → double-click the `.uproject` to relaunch.

For ordinary function-body changes, skip all that and use Live Coding.

### Other Rider things worth knowing

- **Build → Rebuild** — forces a clean compile of everything. Slow, but fixes a whole category of "this makes no sense" states.
- **Attach to Unreal Editor** run configuration — set breakpoints in C++ and debug the running editor. You'll want this sooner than expected.
- **`.sln` caveat:** the solution file is a generated artifact that goes stale as you add classes. Rider can open the `.uproject` directly — the better habit.

---

## 5. Reflection and the Unreal Header Tool

### What UHT actually does

UHT does **not** translate or transform your code. Your `.cpp` and `.h` files go to the compiler completely unmodified. UHT **reads** them and **writes additional C++ files alongside them**.

The macros are **markers**. `UPROPERTY()`, `UCLASS()`, `UFUNCTION()` expand to almost nothing for the actual C++ compiler. Their real audience is UHT, which scans headers for them and treats each as *"record this member in the metadata tables."*

What it generates is a **registration table**: a `UClass` object describing your class at runtime — every reflected field's name, type, byte offset, and flags.

### What reflection is

Reflection is a program's ability to inspect its own structure at runtime — to ask *"what type is this object, what fields does it have, what are their names and types"* and get real answers while running.

**C++ deliberately doesn't have this.** When you write:

```cpp
float TestSpeed = 100.f;
```

the compiler assigns a location and rewrites every use as an offset into the object. The string `"TestSpeed"` exists in your source and in debug symbols, but **in the compiled binary the name is gone**. No runtime table maps names to fields. This is intentional — C++'s design priority is zero overhead, and that metadata would cost memory in every program whether used or not.

### Why Unreal needs it anyway

Select an actor and the Details panel shows a row labeled "Test Speed" with an editable number box. To build that panel the editor must — at runtime, for a class it was never compiled against — enumerate your fields, get names, get types (number box vs checkbox vs asset picker), and read/write actual memory at the right offsets.

Same requirement elsewhere:

| System | Why it needs reflection |
|---|---|
| **Serialization** | Saving a level writes property *names* and values, so it survives you adding or reordering fields |
| **Blueprint** | A node for your C++ function needs parameter names and types to draw pins |
| **Garbage collection** | The collector walks every live object asking "which fields point to other UObjects?" |
| **Networking, undo, property system** | Same underlying metadata |

So Unreal builds reflection itself. UHT generates C++ that constructs, at startup, a `UClass` per class — containing a list of `FProperty` entries, each with a name, type, and byte offset. When the Details panel draws your actor, it's walking that list.

**That's why the macro is unavoidable:** Unreal can't change what C++ compiles to, so it generates the missing metadata as more C++ — and needs you to mark which things to generate it for.

---

## 6. Class prefixes: U, A, F

**The rule: the prefix tells you which memory-management contract the type follows.**

### `U` — UObject subclasses
Garbage-collected. You never `delete` one. Create with `NewObject<UMyThing>()`; the GC reclaims when nothing references them.

**Critically:** "referenced" means *referenced through a `UPROPERTY`*. A raw `UMyThing*` that isn't a `UPROPERTY` is **invisible to the collector** — the object can be destroyed with your pointer still pointing at it. Most common Unreal crash for newcomers. This is why `UPROPERTY()` on an object pointer is about **lifetime**, not just editor visibility.

Examples: `UActorComponent`, `UStaticMesh`.

### `A` — AActor subclasses
These *are* UObjects (`AActor` descends from `UObject`), so everything above applies. The separate letter marks a **capability**: an Actor can be placed or spawned in a level. It has a Transform, holds components, participates in tick and the world. Spawn with `SpawnActor` rather than `NewObject`.

So `A` is a subset of `U` with a stronger contract — the distinct letter is a fast visual signal for *"this lives in the world."*

### `F` — plain C++ structs and classes
Outside the whole system. No GC, no reflection by default, no `UObject` base. Value types: stack or member allocated, copied freely, destroyed by normal C++ scoping.

Examples: `FVector`, `FString`, `FRotator`.

**What an `F` type doesn't get is a managed lifetime. It's yours.**

### The nuance: `USTRUCT`

Reflection is **opt-in**, and reflection and memory management are **separate axes**.

Mark an `F` struct `USTRUCT()` with `UPROPERTY()` members and UHT generates a `UScriptStruct` — enough for the Details panel and serialization. `FVector` is exactly this, which is why you can expand a Location field and edit X, Y, Z.

What a `USTRUCT` still **doesn't** get: a managed lifetime. Not garbage collected, copied by value, dies by normal C++ scoping.

Plain `F` types with no macro have neither — just ordinary C++.

### Practical read
- `U` or `A` → engine owns the lifetime; you need a `UPROPERTY` to hold a reference safely
- `F` → normal C++ rules apply

Also: `I` for interfaces, `E` for enums.

---

## 7. `.generated.h`

It's a real file on disk that UHT writes:

```
Intermediate/Build/Win64/UnrealEditor/Inc/YourProject/UHT/TestbedActor.generated.h
```

Its contents are mostly macro **definitions** — constructor declarations, the `StaticClass()` function, the boilerplate tying your class into the reflection system. Those definitions are what `GENERATED_BODY()` expands to.

```cpp
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TestbedActor.generated.h"   // must be last

UCLASS()
class MYPROJECT_API ATestbedActor : public AActor
{
    GENERATED_BODY()
    // ...
};
```

### The precise mechanism

UHT identifies which class a `GENERATED_BODY()` belongs to by **file position**. It parses the header, notes the line number of the `.generated.h` include, and generates definitions keyed to the class appearing *after* that point. `GENERATED_BODY()` itself expands using the current line number, so the definition it resolves to is the one UHT built for the class at that spot.

**Two consequences:**

1. **Why it must be last.** If another include follows, UHT's line-number bookkeeping desynchronizes from what the compiler sees, and the generated macros don't line up with your class. Errors name your class and reference `GENERATED_BODY` nonsensically — classically, an explicit complaint that the include must be last.

2. **Why one reflected class per header, mostly.** Position-keying is why the convention exists. Not a hard limit — UHT handles multiple `UCLASS`es per file — but one-class-per-header keeps it unambiguous.

**Failure mode to recognize:** errors mentioning `GENERATED_BODY`, `StaticClass`, or unresolved externals for a class you clearly defined usually mean the generated file is stale. Fix is a full rebuild, or deleting `Intermediate` and `Binaries` and regenerating.

---

## 8. Declaring a UPROPERTY — private is correct

Declaring `TestSpeed` under `private` is **idiomatic**, not merely acceptable. Epic's own code does this. Keeping data private and exposing it deliberately is the habit a systems engineer wants; making everything public just to see it in the editor quietly gives away encapsulation.

**Why it works:** `private` is a **compile-time** construct. The compiler enforces it during translation, then it's gone. The property system doesn't call accessors — it reads and writes raw memory at the byte offset in the `FProperty`. Access specifiers are invisible at that layer.

### The gotcha: Blueprint access

```cpp
UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
float TestSpeed = 100.f;
```

Without the `meta` tag, **UHT hard-errors** — it refuses to generate Blueprint accessors for a private member unless you explicitly say you meant it. UHT enforcing a policy the C++ compiler wouldn't care about — a reminder it's a separate tool with its own rules.

### Edit specifier family

| Specifier | Effect |
|---|---|
| `EditAnywhere` | Editable on Blueprint defaults **and** each placed instance |
| `EditDefaultsOnly` | Blueprint class only; level instances can't diverge |
| `EditInstanceOnly` | The reverse |
| `VisibleAnywhere` | Shown but greyed out |

This matters more than it looks. `EditDefaultsOnly` on movement speed says *"this is a property of the character type, not of one copy sitting in this level"* — real design intent expressed in a keyword.

### Closing the loop

```cpp
UE_LOG(LogTemp, Warning, TEXT("TestSpeed is %f"), TestSpeed);
```

Set the value to 250 in the Details panel, play, check the Output Log. Printing 250 rather than the C++ default proves the editor wrote into your object's memory and your code read it back — reflection working end to end, both directions.

---

## 9. The CDO and delta serialization

**Observation:** changed the C++ default to 500, rebuilt, and the placed instance kept 250.

**Mechanism: delta serialization.**

Every `UClass` has a hidden instance called the **Class Default Object (CDO)** — one per class, constructed at startup, holding the defaults from your C++ constructor. Your placed actor doesn't store a full copy of every property. When the level saves, Unreal compares your instance against the CDO and writes down **only the differences**.

Typing 250 flagged that property as overridden and wrote it into the level file. Changing the C++ default changed the CDO — but the instance has an explicit override that wins.

**The corollary worth internalizing:** an **untouched** property would have picked up 500. It isn't stored anywhere, so it resolves through the CDO at load time.

> **Test it:** place a second `ATestbedActor`, don't touch `TestSpeed`, change the C++ default, rebuild. That one *will* change.

**Visible in the UI:** the small **yellow reset arrow** right of the value box *is* the override flag — it only appears on properties differing from the default. Click it and the property reverts to inheriting from the CDO, tracking future C++ changes again.

This is why Unreal can add and reorder properties across engine versions without breaking levels, and it's the same mechanism making Blueprint subclasses cheap. **Worth knowing by name in an interview.**

### Editor access vs Blueprint access

| | Governs | Mechanism |
|---|---|---|
| **`EditAnywhere`** | The Details panel (data-entry grid) | Pure reflection — reads the `FProperty` offset and pokes memory directly. Access specifiers never enter into it. |
| **`BlueprintReadWrite` / `BlueprintReadOnly`** | The Blueprint **graph** — whether you can drag out a `Get`/`Set` node and wire it into logic | Generated accessor code called from a different language runtime |

That's why the `private` rule splits as it does. Details-panel editing bypasses C++ access rules entirely. Blueprint *code* calling your member is conceptually code accessing a private member — UHT refuses to generate it silently, and `AllowPrivateAccess = "true"` is you signing off.

**Note:** making a Blueprint *subclass* of `ATestbedActor` is **not** what "Blueprint access" means. You'll get a Details panel on the Blueprint's defaults — same `EditAnywhere` mechanism, no `BlueprintReadWrite` needed. The keyword only matters once you open the Event Graph and want a node.

> **Worth doing:** add `BlueprintReadWrite` without the meta tag and rebuild. Read the error — it names UHT explicitly, and you'll recognize the tool rather than the compiler talking.

---

## 10. The gameplay framework

### The problem it solves

Naive design: one class, `APlayer`, holding mesh, capsule, movement, camera, input, health, inventory. Reasonable — until real requirements break it:

- Player possesses a turret, then returns to their body. **Where does the score live?** Not the turret. Not the body — that was destroyed and respawned during the possession.
- **Multiplayer.** Ten players, each on their own client. Which parts exist on the server, which on each client, which only on the machine of the human sitting there?
- An **AI enemy** uses the same mesh and movement code as the player. What differs?
- Player dies, pawn destroyed. They're still connected, still on a team, still have a name and ping. **Where does that live?**

Every one is the same shape: **some state belongs to the body, some to the agent controlling it, and those have different lifetimes.**

### The split

| Class | Role | Lifetime / Network |
|---|---|---|
| **`APawn`** | Physical presence — mesh, collision, movement component | Exists in the world; destroyed and respawned freely |
| **`AController`** | The will driving the pawn — receives input, tracks possession | Persists across pawn death. `APlayerController` exists **only on the server and that player's own machine** — never on other clients |
| **`APlayerState`** | Persistent per-player data everyone sees — name, score, team, ping | **Replicated to all clients** (this is how a scoreboard works) |
| **`AGameModeBase`** | The rules — who spawns where, what pawn class, when the match ends | **Server-only** |
| **`AGameStateBase`** | Match state everyone needs — time remaining, phase | Replicated companion to GameMode |

`ACharacter` is the specialized `APawn` subclass adding a capsule, skeletal mesh, and `UCharacterMovementComponent` — walking, falling, crouching, jumping, and network-corrected movement for free. Thousands of lines of hard-won code.

**Possession is the join:** `Controller->Possess(Pawn)`. One call and input routes from that controller into that pawn. Swap the pawn, keep the controller → vehicle entry, spectating, mind control, with no special-case code.

### Placement exercise — answers

**1. Current health → the Pawn.**

Two problems with putting it on `APlayerController`:
- **AI enemies need health too**, and they have an `AAIController`. Health on the player's controller means a parallel implementation or a shared base — splitting a concept that should be one thing. Whereas *every* damaged thing is a Pawn (or at least an Actor).
- **Decisive:** `APlayerController` isn't replicated to other clients. Health there means **no other player can ever see your health bar** — no floating bars, team HUDs, or spectator views.

`AActor::TakeDamage()` is defined at the Actor level — that tells you where Epic thinks damage lives.

*On the spectating intuition:* right idea, arrow backwards. The Pawn dies and **broadcasts**; the Controller **listens** and reacts by unpossessing and switching to spectate. The controller responds to death — it doesn't own the number.

*Legitimate variant:* some games put health on `PlayerState` so it survives respawn and replicates to everyone (Lyra does something like this). Deliberate choice for persistence, not the default.

**2. Jump input binding → split across two objects.**

- **Which key maps to which action** (the Input Mapping Context) is applied through the PlayerController's local player subsystem — per-human, rebindable, controller side.
- **What the action does** (the actual `Jump()` call) is bound in the **Pawn**, in `SetupPlayerInputComponent()`.

**That split is the payoff.** The IMC says "Space means IA_Jump." The character says "IA_Jump means jump." Possess a vehicle and IA_Jump now means handbrake — same key, same action asset, different pawn interpreting it. Context-sensitive controls with no branching logic.

**3. Walk speed → the Pawn.** ✅ Correct — movement speed differs per actor and per effect.

**4. Camera → the Pawn**, plus a missing piece.

Camera components on the Pawn is correct and is what Epic's templates do (SpringArm + Camera on the Character). Vehicle-vs-turret reasoning is exactly why.

**The subtlety:** the camera *component* is on the pawn, but **where you're looking** is not. `AController::ControlRotation` lives on the **controller** — aim direction should survive changing pawns, and an AI controller needs a "facing" concept with no camera at all.

You'll meet this as **`bUseControllerRotationYaw`** on your Character: *"should my body turn to match where the controller is looking, or turn toward where I'm moving?"* That single boolean is the difference between a third-person shooter and an adventure-game character — and it exists because rotation-intent and body-rotation are separate things owned by separate objects.

Also: `APlayerController` holds a `PlayerCameraManager` owning the actual view. Normally it just reads the pawn's camera component, but it's what makes view blending, camera shakes, and cinematic view switching possible.

**5. Total kills this match → `APlayerState`.** ✅ Correct — accessible to all clients and the server.

**6. Respawn-after-5-seconds rule → `AGameModeBase`.** ✅ Correct. One refinement: the enforcement isn't "clients can't modify it" so much as **GameMode doesn't exist on clients at all** — server-only, nothing to tamper with.

---

## 11. ACharacter vs custom APawn — the decision

**Tempting instinct:** subclass `APawn` and implement movement from scratch, because it's harder and therefore more impressive.

**Pushback — the recruiter-perception premise is wrong.**

An experienced systems engineer doesn't see "reimplemented movement" as ambition. They see a **decision** and immediately ask *why*. If the answer is "to show I could," the read is often negative — reinventing a battle-tested engine subsystem without cause is the exact instinct flagged in code review. **Knowing when *not* to build something is a senior trait.**

**The harder problem underneath:** `UCharacterMovementComponent` isn't mostly "make the capsule move." It's:
- client-side prediction
- server reconciliation
- replaying unacknowledged moves when the server disagrees
- step-up onto ledges, slope limits, ground-vs-falling state
- penetration resolution, network smoothing

A from-scratch APawn ignoring all that isn't a more impressive CMC — it's a **visibly worse one**, noticeable within thirty seconds of play by anyone qualified to evaluate you.

**What actually reads as strong:** *can this person work productively inside a large existing system they didn't write?* That's 95% of the job. Nobody at a studio writes a movement component from nothing.

### When custom APawn *is* right

If the premise is movement CMC genuinely doesn't serve — a spacecraft with 6DOF, a spherical rolling physics body, a fully flight-based creature — it's the correct engineering decision and defensible in a sentence. Different project than "character controller demo."

Middle option worth knowing: **`UFloatingPawnMovement`** — a lightweight movement component giving basic velocity/acceleration handling on a plain APawn.

### The recommended path

> **Subclass `ACharacter`, then subclass `UCharacterMovementComponent`** and add a movement ability CMC doesn't ship with — a dash, wall-run, mantle, or grapple.

Harder than it sounds, in the ways that matter. You'll work with `MOVE_Custom` and `PhysCustom()`, and to make it network-correct you'll touch `FSavedMove_Character`:
- `CanCombineWith`
- `SetMoveFor`
- `PrepMoveFor`
- compressed-flags packing
- `FNetworkPredictionData_Client_Character::AllocateNewMove`

Real systems work inside a real system. It's the single most common "show me something you built" answer that makes gameplay engineers lean forward, because it proves you read engine source and understood a prediction model rather than avoiding it.

You get the depth, you don't spend two weeks rebuilding step-up logic worse than Epic did, and the thing is actually playable when a recruiter opens it.

---

## 12. Signature feature: the charged dash

**Chosen:** a dash whose distance scales with how long the input is held before release.

**Why this is better than a plain dash — and it's not aesthetic:**

| | Plain dash | Charged dash |
|---|---|---|
| Data | A **boolean** — did you dash this frame | **Variable magnitude** |
| Network cost | One bit in the compressed flags byte | A *number* client and server must agree on |
| What it forces | Nearly free sync | The number must survive being packed into a saved move, replayed during correction, and combined (or not) with adjacent moves |

The default flag-packing doesn't cover variable magnitude — **you have to extend `FSavedMove_Character` yourself.** That's the part that actually teaches the prediction model.

### Build order

Get each working before starting the next:

1. **`ACharacter` subclass that walks and looks** with Enhanced Input
2. **Jump** — nearly free, `ACharacter::Jump()` already exists
3. **Custom movement mode + charged dash**

Step 1 is where you meet every Unreal idiom you need: components, the input subsystem, delegates, `SetupPlayerInputComponent`. **Rushing past it to get to the interesting part is the most common way this project stalls.**

### Assets to create

**Input Actions:**
- `IA_Move` — Value Type: Axis2D
- `IA_Look` — Axis2D
- `IA_Jump` — Digital (bool)
- `IA_Dash` — Digital (bool)

**Input Mapping Context** — `IMC_Default`: WASD → `IA_Move`, mouse XY → `IA_Look`, Space → `IA_Jump`, Shift → `IA_Dash`

### Open question to hold

`BindAction` takes an `ETriggerEvent`. The values: `Started`, `Triggered`, `Ongoing`, `Completed`, `Canceled`.

A charged dash needs to know when the button went down and when it came up. Which events give you those?

Then: **do you accumulate charge by storing a timestamp on press and subtracting on release, or by adding `DeltaTime` every frame while held?**

> These feel equivalent right now. **They are not equivalent once the server is replaying your moves during a correction.** Don't solve it yet — hold the question while building step 1.

---

## 13. `IA_Move` walkthrough

### First: the module dependency

`Source/YourProject/YourProject.Build.cs`:

```csharp
PublicDependencyModuleNames.AddRange(new string[] { 
    "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput" 
});
```

Skip this and your code **compiles but fails at link time** with unresolved externals for every Enhanced Input symbol. Most common stumble here; the error looks scarier than it is. Changing a `.Build.cs` requires a **full rebuild**, not Live Coding.

### The assets

Make `Content/Input`.

**Input Action:** right-click → Input → Input Action. Name `IA_Move`. Set **Value Type** to `Axis2D (Vector2D)`. Only field that matters right now.

**Input Mapping Context:** right-click → Input → Input Mapping Context. Name `IMC_Default`. Add a mapping set to `IA_Move`, then add **four key bindings** under that one action — W, A, S, D.

### Modifiers

A key press natively produces scalar `1.0`, landing on the **X** axis. Route each key to the right axis with the right sign:

| Key | Modifiers | Result |
|---|---|---|
| **D** | none | (1, 0) |
| **A** | Negate | (-1, 0) |
| **W** | Swizzle Input Axis Values (YXZ) | (0, 1) |
| **S** | Swizzle (YXZ) + Negate | (0, -1) |

Swizzle permutes the components — `YXZ` moves the X value into the Y slot. Result: **X = strafe, Y = forward/back**, the convention Epic's templates use. **Order matters on S:** swizzle first, then negate.

> Worth noticing what you just did — you expressed "WASD means 2D movement" as **data**, with zero branching logic. Rebinding to arrow keys or a gamepad stick is now an asset edit, not a code change.

### The C++ side

**Header — hold pointers to the assets:**

```cpp
UPROPERTY(EditDefaultsOnly, Category = "Input")
TObjectPtr<UInputMappingContext> DefaultMappingContext;

UPROPERTY(EditDefaultsOnly, Category = "Input")
TObjectPtr<UInputAction> MoveAction;
```

`EditDefaultsOnly` because these are properties of the character **type**, not one level instance. `TObjectPtr` is the modern replacement for raw pointers on UPROPERTYs — treat it as a `UInputAction*` for now.

**Apply the mapping context** — in `BeginPlay()`:

```cpp
if (APlayerController* PC = Cast<APlayerController>(Controller))
{
    if (UEnhancedInputLocalPlayerSubsystem* Subsystem = 
        ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
    {
        Subsystem->AddMappingContext(DefaultMappingContext, 0);
    }
}
```

The `0` is **priority**. Higher numbers win when contexts conflict — how you'd later push an `IMC_Vehicle` on top that overrides some keys and leaves others alone.

**Bind the action** — in `SetupPlayerInputComponent()`:

```cpp
if (UEnhancedInputComponent* EIC = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
{
    EIC->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AMyCharacter::Move);
}
```

**The handler:**

```cpp
void AMyCharacter::Move(const FInputActionValue& Value)
{
    const FVector2D MovementVector = Value.Get<FVector2D>();
    // ...
}
```

**Includes needed:** `EnhancedInputComponent.h`, `EnhancedInputSubsystems.h`, `InputActionValue.h`.

### The part to reason through

You now have a 2D vector. `AddMovementInput(FVector Direction, float Scale)` wants a **world-space direction** and a magnitude.

The naive move — `AddMovementInput(GetActorForwardVector(), MovementVector.Y)` — works, but W always moves you where the **body** faces, so turning the camera doesn't change where you walk. For third-person you want W to mean *"away from the camera."*

So the direction comes from the **controller's rotation**, not the actor's — the ControlRotation-lives-on-the-Controller point showing up in real code.

Tool for converting a rotation into a direction vector:

```cpp
FRotationMatrix(SomeRotator).GetUnitAxis(EAxis::X)
```

> **Trap to reason about first:** `GetControlRotation()` returns pitch, yaw, **and** roll. If you feed all three in, what happens when the player looks at the ground and presses W? Figure out which components to keep and you'll have written the line yourself.

---

## Open threads

- [ ] `ETriggerEvent` — which events capture press and release for the charged dash
- [ ] Charge accumulation: timestamp-on-press vs `DeltaTime`-per-frame — and why they diverge under server correction
- [ ] `GetControlRotation()` — which rotation components to keep for ground movement
- [ ] Extending `FSavedMove_Character` for variable-magnitude dash replication
