#include "Subsystems/HelperClasses/KBFL_CDOHelperClass_CategoryOrder.h"

#include "FGCategory.h"
#include "FGItemCategory.h"
#include "Subsystems/KBFLAssetDataSubsystem.h"
#include "Subsystems/KBFLContentCDOHelperSubsystem.h"

void UKBFL_CDOHelperClass_CategoryOrder::DoCDO()
{
	if (!IsValidSoftClass(mTargetCategory) || !IsValidSoftClass(mSubTargetCategory))
	{
		return;
	}

	TSubclassOf<UFGBuildCategory> TargetCategory = mTargetCategory.LoadSynchronous();
	TSubclassOf<UFGBuildSubCategory> TargetSubCategory = mSubTargetCategory.LoadSynchronous();

	TArray<UClass*> Classes = GetClasses();

	if (mTargetCategoryPrio > -1000.f)
	{
		UFGBuildCategory* DefaultCategoryObject =
			mSubsystem->GetAndStoreDefaultObject_Native<UFGBuildCategory>(TargetCategory);
		DefaultCategoryObject->mMenuPriority = mTargetCategoryPrio;
	}

	if (mSubTargetCategoryPrio > -1000.f)
	{
		UFGBuildSubCategory* DefaultSubCategoryObject =
			mSubsystem->GetAndStoreDefaultObject_Native<UFGBuildSubCategory>(TargetSubCategory);
		DefaultSubCategoryObject->mMenuPriority = mSubTargetCategoryPrio;
	}

	for (int32 Index = 0; Index < Classes.Num(); Index++)
	{
		if (TSubclassOf<UFGBuildDescriptor> Class = Classes[Index])
		{
			UFGBuildDescriptor* DefaultObject = mSubsystem->GetAndStoreDefaultObject_Native<UFGBuildDescriptor>(Class);

			DefaultObject->mCategory = TargetCategory;
			DefaultObject->mSubCategories.Empty();
			DefaultObject->mSubCategories.Add(TargetSubCategory);
			DefaultObject->mMenuPriority = Index;
		}
	}

	Super::DoCDO();
}

TArray<UClass*> UKBFL_CDOHelperClass_CategoryOrder::GetClasses()
{
	TArray<UClass*> Re;

	for (TSoftClassPtr<UFGBuildDescriptor> TargetOrder : mTargetOrder)
	{
		if (IsValidSoftClass(TargetOrder))
		{
			Re.Add(TargetOrder.LoadSynchronous());
		}
	}

	return Re;
}
