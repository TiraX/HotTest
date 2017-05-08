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


const int grid_size = 512;

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

	FOceanMeshSceneProxy(UOceanMeshComponent* Component)
		: FPrimitiveSceneProxy(Component)
		, MaterialRelevance(Component->GetMaterialRelevance(GetScene().GetFeatureLevel()))
	{
		const FColor VertexColor(255, 255, 255);

		VertexBuffer.Vertices.AddUninitialized(grid_size * grid_size);
		IndexBuffer.Indices.AddUninitialized((grid_size - 1) * (grid_size - 1) * 2 * 3);
		// construct grid
		for (int y = 0 ; y < grid_size ; ++ y)
		{
			for (int x = 0 ; x < grid_size ; ++ x)
			{
			}
		}
		//for (int32 TriIdx = 0; TriIdx < NumTris; TriIdx++)
		//{
		//	FOceanMeshTriangle& Tri = Component->OceanMeshTris[TriIdx];

		//	const FVector Edge01 = (Tri.Vertex1 - Tri.Vertex0);
		//	const FVector Edge02 = (Tri.Vertex2 - Tri.Vertex0);

		//	const FVector TangentX = Edge01.GetSafeNormal();
		//	const FVector TangentZ = (Edge02 ^ Edge01).GetSafeNormal();
		//	const FVector TangentY = (TangentX ^ TangentZ).GetSafeNormal();

		//	FDynamicMeshVertex Vert;

		//	Vert.Color = VertexColor;
		//	Vert.SetTangents(TangentX, TangentY, TangentZ);

		//	Vert.Position = Tri.Vertex0;
		//	VertexBuffer.Vertices[TriIdx * 3 + 0] = Vert;
		//	IndexBuffer.Indices[TriIdx * 3 + 0] = TriIdx * 3 + 0;

		//	Vert.Position = Tri.Vertex1;
		//	VertexBuffer.Vertices[TriIdx * 3 + 1] = Vert;
		//	IndexBuffer.Indices[TriIdx * 3 + 1] = TriIdx * 3 + 1;

		//	Vert.Position = Tri.Vertex2;
		//	VertexBuffer.Vertices[TriIdx * 3 + 2] = Vert;
		//	IndexBuffer.Indices[TriIdx * 3 + 2] = TriIdx * 3 + 2;
		//}

		//// Init vertex factory
		//VertexFactory.Init(&VertexBuffer);

		//// Enqueue initialization of render resource
		//BeginInitResource(&VertexBuffer);
		//BeginInitResource(&IndexBuffer);
		//BeginInitResource(&VertexFactory);

		//// Grab material
		//Material = Component->GetMaterial(0);
		//if (Material == NULL)
		//{
		//	Material = UMaterial::GetDefaultMaterial(MD_Surface);
		//}
	}

	virtual ~FOceanMeshSceneProxy()
	{
		VertexBuffer.ReleaseResource();
		IndexBuffer.ReleaseResource();
		VertexFactory.ReleaseResource();
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

UOceanMeshComponent::UOceanMeshComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = false;

	SetCollisionProfileName(UCollisionProfile::BlockAllDynamic_ProfileName);
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
	NewBounds.Origin = FVector::ZeroVector;
	NewBounds.BoxExtent = FVector(HALF_WORLD_MAX, HALF_WORLD_MAX, HALF_WORLD_MAX);
	NewBounds.SphereRadius = FMath::Sqrt(3.0f * FMath::Square(HALF_WORLD_MAX));
	return NewBounds;
}




