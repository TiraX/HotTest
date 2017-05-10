// Fill out your copyright notice in the Description page of Project Settings.

#include "HoudiniOceanTest.h"
#include "OceanMeshComponent.h"
#include "RenderingThread.h"
#include "RenderResource.h"
#include "PrimitiveViewRelevance.h"
#include "PrimitiveSceneProxy.h"
#include "VertexFactory.h"
#include "MaterialShared.h"
#include "Engine/CollisionProfile.h"
#include "Materials/Material.h"
#include "LocalVertexFactory.h"
#include "SceneManagement.h"
#include "DynamicMeshBuilder.h"
#include "EngineGlobals.h"
#include "Engine/Engine.h"
#include "Ocean.h"


const int _grid_size = 512;

/** Vertex Buffer */
class FOceanMeshVertexBuffer : public FVertexBuffer
{
public:
	TArray<FDynamicMeshVertex> Vertices;

	virtual void InitRHI() override
	{
		FRHIResourceCreateInfo CreateInfo;
		void* VertexBufferData = nullptr;
		VertexBufferRHI = RHICreateAndLockVertexBuffer(Vertices.Num() * sizeof(FDynamicMeshVertex), BUF_Static, CreateInfo, VertexBufferData);

		// Copy the vertex data into the vertex buffer.		
		FMemory::Memcpy(VertexBufferData, Vertices.GetData(), Vertices.Num() * sizeof(FDynamicMeshVertex));
		RHIUnlockVertexBuffer(VertexBufferRHI);
	}

};

/** Index Buffer */
class FOceanMeshIndexBuffer : public FIndexBuffer
{
public:
	TArray<int32> Indices;

	virtual void InitRHI() override
	{
		FRHIResourceCreateInfo CreateInfo;
		void* Buffer = nullptr;
		IndexBufferRHI = RHICreateAndLockIndexBuffer(sizeof(int32), Indices.Num() * sizeof(int32), BUF_Static, CreateInfo, Buffer);

		// Write the indices to the index buffer.		
		FMemory::Memcpy(Buffer, Indices.GetData(), Indices.Num() * sizeof(int32));
		RHIUnlockIndexBuffer(IndexBufferRHI);
	}
};

/** Vertex Factory */
class FOceanMeshVertexFactory : public FLocalVertexFactory
{
public:

	FOceanMeshVertexFactory()
	{}

	/** Init function that should only be called on render thread. */
	void Init_RenderThread(const FOceanMeshVertexBuffer* VertexBuffer)
	{
		check(IsInRenderingThread());

		FDataType NewData;
		NewData.PositionComponent = STRUCTMEMBER_VERTEXSTREAMCOMPONENT(VertexBuffer, FDynamicMeshVertex, Position, VET_Float3);
		NewData.TextureCoordinates.Add(
			FVertexStreamComponent(VertexBuffer, STRUCT_OFFSET(FDynamicMeshVertex, TextureCoordinate), sizeof(FDynamicMeshVertex), VET_Float2)
		);
		NewData.TangentBasisComponents[0] = STRUCTMEMBER_VERTEXSTREAMCOMPONENT(VertexBuffer, FDynamicMeshVertex, TangentX, VET_PackedNormal);
		NewData.TangentBasisComponents[1] = STRUCTMEMBER_VERTEXSTREAMCOMPONENT(VertexBuffer, FDynamicMeshVertex, TangentZ, VET_PackedNormal);
		NewData.ColorComponent = STRUCTMEMBER_VERTEXSTREAMCOMPONENT(VertexBuffer, FDynamicMeshVertex, Color, VET_Color);

		SetData(NewData);
	}

	/** Initialization */
	void Init(const FOceanMeshVertexBuffer* VertexBuffer)
	{
		if (IsInRenderingThread())
		{
			Init_RenderThread(VertexBuffer);
		}
		else
		{
			ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
				InitOceanMeshVertexFactory,
				FOceanMeshVertexFactory*, VertexFactory, this,
				const FOceanMeshVertexBuffer*, VertexBuffer, VertexBuffer,
				{
					VertexFactory->Init_RenderThread(VertexBuffer);
				});
		}
	}
};

/** Scene proxy */
class FOceanMeshSceneProxy : public FPrimitiveSceneProxy
{
public:

	drw::Ocean*	_Ocean;
	drw::OceanContext*	_OceanContext;

	FOceanMeshSceneProxy(UOceanMeshComponent* Component)
		: FPrimitiveSceneProxy(Component)
		, MaterialRelevance(Component->GetMaterialRelevance(GetScene().GetFeatureLevel()))
		, _Ocean(nullptr)
		, _OceanContext(nullptr)
	{
		const FColor VertexColor(255, 255, 255);

		VertexBuffer.Vertices.AddUninitialized(Component->GridSize * Component->GridSize);
		IndexBuffer.Indices.AddUninitialized((Component->GridSize - 1) * (Component->GridSize - 1) * 2 * 3);

		// create things
		int gridres = 1 << Component->GridRes;
		float stepsize = Component->GridSize / (float)gridres;
		_Ocean = new drw::Ocean(gridres, gridres, stepsize, stepsize,
			Component->WindSpeed, Component->ShortestWaveLength, Component->WaveHeight, Component->WindDirection, 
			Component->Damp, Component->WindAlign, Component->Depth, 12306);
		float OceanScale = _Ocean->get_height_normalize_factor();

		_OceanContext = _Ocean->new_context(true, Component->bChop, true, Component->bJacobian);


		// sum up the waves at this timestep
		_Ocean->update(0.f, *_OceanContext, true, Component->bChop, true, Component->bJacobian, OceanScale, Component->Choppyness);

		const float gridScale = _grid_size / Component->GridSize;

		// construct grid
		const float uv_step = 1.f / Component->GridSize;
		for (int y = 0 ; y < Component->GridSize; ++ y)
		{
			for (int x = 0 ; x < Component->GridSize; ++ x)
			{
				FDynamicMeshVertex vertex;
				vertex.Position = FVector(x * gridScale, y * gridScale, 0.f);
				_OceanContext->eval2_xz(x * gridScale, y * gridScale);

				vertex.Position.X += _OceanContext->disp[0] * Component->WaveScale;
				vertex.Position.Y += _OceanContext->disp[2] * Component->WaveScale;
				vertex.Position.Z += _OceanContext->disp[1] * Component->WaveScale;

				vertex.TextureCoordinate = FVector2D(x * uv_step, y * uv_step);

				FVector TangentZ = FVector(_OceanContext->normal[0], _OceanContext->normal[2], _OceanContext->normal[1]);
				TangentZ.Normalize();
				const FVector VecHor = FVector(1, 0, 0);
				const FVector TangentY = (VecHor ^ TangentZ).GetSafeNormal();
				const FVector TangentX = (TangentY ^ TangentZ).GetSafeNormal();

				//const FVector TangentZ = FVector(0, 0, 1);
				//const FVector TangentX = FVector(1, 0, 0);
				//const FVector TangentY = (TangentX ^ TangentZ).GetSafeNormal();

				vertex.SetTangents(TangentX, TangentY, TangentZ);
				vertex.Color = FColor::Black;

				VertexBuffer.Vertices[y * Component->GridSize + x] = vertex;
			}
		}
		int index = 0;
		for (int y = 0; y < Component->GridSize - 1; ++y)
		{
			for (int x = 0; x < Component->GridSize - 1; ++x)
			{
				IndexBuffer.Indices[index++] = y * Component->GridSize + x;
				IndexBuffer.Indices[index++] = (y + 1) * Component->GridSize + x;
				IndexBuffer.Indices[index++] = y * Component->GridSize + x + 1;

				IndexBuffer.Indices[index++] = (y + 1) * Component->GridSize + x + 1;
				IndexBuffer.Indices[index++] = y * Component->GridSize + x + 1;
				IndexBuffer.Indices[index++] = (y + 1) * Component->GridSize + x;
			}
		}

		// Init vertex factory
		VertexFactory.Init(&VertexBuffer);

		// Enqueue initialization of render resource
		BeginInitResource(&VertexBuffer);
		BeginInitResource(&IndexBuffer);
		BeginInitResource(&VertexFactory);

		// Grab material
		Material = Component->GetMaterial(0);
		if (Material == NULL)
		{
			Material = UMaterial::GetDefaultMaterial(MD_Surface);
		}
	}

	virtual ~FOceanMeshSceneProxy()
	{
		VertexBuffer.ReleaseResource();
		IndexBuffer.ReleaseResource();
		VertexFactory.ReleaseResource();

		if (_OceanContext)
		{
			delete _OceanContext;
		}
		if (_Ocean)
		{
			delete _Ocean;
		}
	}

	virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override
	{
		QUICK_SCOPE_CYCLE_COUNTER(STAT_OceanMeshSceneProxy_GetDynamicMeshElements);

		const bool bWireframe = AllowDebugViewmodes() && ViewFamily.EngineShowFlags.Wireframe;

		auto WireframeMaterialInstance = new FColoredMaterialRenderProxy(
			GEngine->WireframeMaterial ? GEngine->WireframeMaterial->GetRenderProxy(IsSelected()) : NULL,
			FLinearColor(0, 0.5f, 1.f)
		);

		Collector.RegisterOneFrameMaterialProxy(WireframeMaterialInstance);

		FMaterialRenderProxy* MaterialProxy = NULL;
		if (bWireframe)
		{
			MaterialProxy = WireframeMaterialInstance;
		}
		else
		{
			MaterialProxy = Material->GetRenderProxy(IsSelected());
		}

		for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
		{
			if (VisibilityMap & (1 << ViewIndex))
			{
				const FSceneView* View = Views[ViewIndex];
				// Draw the mesh.
				FMeshBatch& Mesh = Collector.AllocateMesh();
				FMeshBatchElement& BatchElement = Mesh.Elements[0];
				BatchElement.IndexBuffer = &IndexBuffer;
				Mesh.bWireframe = bWireframe;
				Mesh.VertexFactory = &VertexFactory;
				Mesh.MaterialRenderProxy = MaterialProxy;
				BatchElement.PrimitiveUniformBuffer = CreatePrimitiveUniformBufferImmediate(GetLocalToWorld(), GetBounds(), GetLocalBounds(), true, UseEditorDepthTest());
				BatchElement.FirstIndex = 0;
				BatchElement.NumPrimitives = IndexBuffer.Indices.Num() / 3;
				BatchElement.MinVertexIndex = 0;
				BatchElement.MaxVertexIndex = VertexBuffer.Vertices.Num() - 1;
				Mesh.ReverseCulling = IsLocalToWorldDeterminantNegative();
				Mesh.Type = PT_TriangleList;
				Mesh.DepthPriorityGroup = SDPG_World;
				Mesh.bCanApplyViewModeOverrides = false;
				Collector.AddMesh(ViewIndex, Mesh);
			}
		}
	}

	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const override
	{
		FPrimitiveViewRelevance Result;
		Result.bDrawRelevance = IsShown(View);
		Result.bShadowRelevance = IsShadowCast(View);
		Result.bDynamicRelevance = true;
		Result.bRenderInMainPass = ShouldRenderInMainPass();
		Result.bUsesLightingChannels = GetLightingChannelMask() != GetDefaultLightingChannelMask();
		Result.bRenderCustomDepth = ShouldRenderCustomDepth();
		MaterialRelevance.SetPrimitiveViewRelevance(Result);
		return Result;
	}

	virtual bool CanBeOccluded() const override
	{
		return !MaterialRelevance.bDisableDepthTest;
	}

	virtual uint32 GetMemoryFootprint(void) const override { return(sizeof(*this) + GetAllocatedSize()); }

	uint32 GetAllocatedSize(void) const { return(FPrimitiveSceneProxy::GetAllocatedSize()); }

private:

	UMaterialInterface* Material;
	FOceanMeshVertexBuffer VertexBuffer;
	FOceanMeshIndexBuffer IndexBuffer;
	FOceanMeshVertexFactory VertexFactory;

	FMaterialRelevance MaterialRelevance;
};

//////////////////////////////////////////////////////////////////////////
#pragma comment (lib, "libfftw3-3.lib")
#pragma comment (lib, "libfftw3f-3.lib")
#pragma comment (lib, "libfftw3l-3.lib")


UOceanMeshComponent::UOceanMeshComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, bChop(true)
	, bJacobian(true)
	, GridRes(8)
	, GridSize(512)
	, WindSpeed(30.f)
	, WindDirection(0.f)
	, WaveHeight(50.f)
	, ShortestWaveLength(0.05f)
	, Damp(0.6f)
	, WindAlign(2)
	, Depth(200.f)
	, Choppyness(1.6f)
	, WaveScale(1.f)
{
	PrimaryComponentTick.bCanEverTick = false;

	SetCollisionProfileName(UCollisionProfile::BlockAllDynamic_ProfileName);

}

UOceanMeshComponent::~UOceanMeshComponent()
{
}

FPrimitiveSceneProxy* UOceanMeshComponent::CreateSceneProxy()
{
	FPrimitiveSceneProxy* Proxy = new FOceanMeshSceneProxy(this);
	return Proxy;
}

int32 UOceanMeshComponent::GetNumMaterials() const
{
	return 1;
}


FBoxSphereBounds UOceanMeshComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	FBoxSphereBounds NewBounds;
	NewBounds.Origin = FVector(0.f, 0.f, 0.f);
	NewBounds.BoxExtent = FVector(_grid_size, _grid_size, 1.f);
	NewBounds.SphereRadius = FMath::Sqrt(3.0f * FMath::Square(_grid_size));
	return NewBounds;
}




