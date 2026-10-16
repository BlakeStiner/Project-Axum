// Optional probe only. Never referenced by ProjectAxum.gproj.
[BaseContainerProps(category: "Autotest")]
class AX_DrugsEconomyResourceSuite : SCR_AutotestSuiteBase
{
 override ResourceName GetWorldFile() { return ResourceName.Empty; }
}

[Test(suite: AX_DrugsEconomyResourceSuite, timeoutS: 10)]
class AX_DrugsEconomyResourceTest : SCR_AutotestCaseBase
{
 [TestStep(TestStage.Main)]
 void Execute()
 {
  // These are references actually used by the installed package, including
  // its menu preset's older New2 path. GUID redirection is tested as supplied.
  array<ResourceName> resources = {
   "{3AAF86FBB52B7FB1}Prefabs/Items/Drugs/CannabisExtract.et",
   "{ABAD01FB5D7FC9EA}Prefabs/World/Plants/CannabisPlant_Mature.et",
   "{60033EC611BD426D}Prefabs/Items/Currency/Bill_100.et",
   "{2A5E837DC8C796B5}Prefabs/Items/Currency/Bill_200.et",
   "{3397C82A587B73B7}Prefabs/Items/Currency/Bill_500.et",
   "{2BFC5B5C40CFD82D}Prefabs/Items/Currency/Bill_1000.et",
   "{33A9A03B18323BF9}UI/Layouts/Markets/Layouts/BM_ItemRow.layout",
   "{D38DDB4EC9E4EED3}UI/Layouts/Markets/New2/BM_Main.layout"
  };
  foreach (ResourceName name : resources)
  {
   Resource resource = Resource.Load(name);
   if (!resource || !resource.IsValid())
   {
    SetFailure(SCR_AutotestFailure.Create("Resource did not resolve: " + name));
    return;
   }
   Print("AX_DE_RESOURCE_OK " + name);
  }
 }
}
