#include "catch2/catch_test_macros.hpp"

#include "datamodel/ExampleHit.h"
#include "datamodel/ExampleHitCollection.h"
#include "datamodel/MutableExampleCluster.h"
#include "datamodel/MutableExampleMC.h"
#include "datamodel/TypeWithEnergy.h"

#include "interface_extension_model/AnotherHit.h"
#include "interface_extension_model/EnergyInterface.h"
#include "interface_extension_model/MutableAnotherHit.h"

#include "podio/ObjectID.h"
#include "podio/utilities/TypeHelpers.h"

#include <map>
#include <stdexcept>
#include <utility>
#include <vector>

TEST_CASE("InterfaceTypes basic functionality", "[interface-types][basics]") {
  using WrapperT = TypeWithEnergy;

  auto emptyWrapper = WrapperT::makeEmpty();
  REQUIRE_FALSE(emptyWrapper.isAvailable());
  REQUIRE(emptyWrapper.isA<ExampleHit>());

  ExampleHit hit{};
  WrapperT wrapper1 = hit;
  WrapperT wrapper2 = hit;

  // The operator== compares the underlying pointers
  REQUIRE(wrapper1 == wrapper2);
  // The comparison operator is symmetric
  REQUIRE(hit == wrapper1);
  // Reassigning to a different entity should make comparisons fail
  wrapper2 = ExampleHit{};
  REQUIRE(wrapper1 != wrapper2);

  // Comparisons also work with Mutable types
  MutableExampleHit mutHit{};
  wrapper2 = mutHit;
  REQUIRE(mutHit != wrapper1);
  REQUIRE(wrapper2 == mutHit);

  // Make sure that the object id functionality work as expected. The wrapped
  // object is in no collection, so it should be the default id.
  REQUIRE(wrapper1.id() == podio::ObjectID{});

  // Create an element in a collection to get one with a non-default object id
  ExampleHitCollection hitColl{};
  hitColl.setID(42);
  wrapper1 = hitColl.create();
  REQUIRE(wrapper1.id() == podio::ObjectID{0, 42});
}

TEST_CASE("InterfaceTypes static checks", "[interface-types][static-checks]") {
  using namespace std::string_view_literals;
  STATIC_REQUIRE(podio::detail::isInterfaceType<TypeWithEnergy>);
  STATIC_REQUIRE(TypeWithEnergy::typeName == "TypeWithEnergy"sv);
}

TEST_CASE("InterfaceTypes STL usage", "[interface-types][basics]") {
  // Make sure that interface types can be used with STL map and set
  std::map<TypeWithEnergy, int> counterMap{};

  auto empty = TypeWithEnergy::makeEmpty();
  counterMap[empty]++;

  ExampleHit hit{};
  auto wrapper = TypeWithEnergy{hit};
  counterMap[wrapper]++;

  // No way this implicit conversion could ever lead to a subtle bug ;)
  counterMap[hit]++;

  REQUIRE(counterMap[empty] == 1);
  REQUIRE(counterMap[hit] == 2);
  REQUIRE(counterMap[wrapper] == 2);

  auto mc = MutableExampleMC{};
  mc.energy() = 3.14f;

  // check container of interfaces move constructor and direct initialization
  auto interfaces = std::vector<TypeWithEnergy>{empty, hit, mc};
  auto interfaces2 = std::vector<TypeWithEnergy>{std::move(interfaces)};
  REQUIRE_FALSE(interfaces2.at(0).isAvailable());
  REQUIRE(interfaces2.at(1).isA<ExampleHit>());
  REQUIRE(interfaces2.at(2).energy() == 3.14f);
}

TEST_CASE("InterfaceType from immutable", "[interface-types][basics]") {
  using WrapperT = TypeWithEnergy;

  ExampleHit hit{};
  WrapperT wrapper{hit};
  REQUIRE(wrapper.isA<ExampleHit>());
  REQUIRE_FALSE(wrapper.isA<ExampleCluster>());
  REQUIRE(wrapper.as<ExampleHit>() == hit);
  REQUIRE(wrapper == hit);

  ExampleCluster cluster{};
  wrapper = cluster;
  REQUIRE(wrapper.isA<ExampleCluster>());
  REQUIRE(wrapper.as<ExampleCluster>() == cluster);
  REQUIRE_THROWS_AS(wrapper.as<ExampleHit>(), std::runtime_error);
  REQUIRE(wrapper != hit);
}

TEST_CASE("InterfaceType from mutable", "[interface-types][basics]") {
  using WrapperT = TypeWithEnergy;

  ExampleHit hit{};
  WrapperT wrapper{hit};
  REQUIRE(wrapper.isA<ExampleHit>());
  REQUIRE_FALSE(wrapper.isA<ExampleCluster>());
  REQUIRE(wrapper.as<ExampleHit>() == hit);
  REQUIRE(wrapper == hit);
  // Comparison also work against the immutable classes
  ExampleHit immutableHit = hit;
  REQUIRE(wrapper == immutableHit);

  MutableExampleCluster cluster{};
  wrapper = cluster;
  REQUIRE(wrapper.isA<ExampleCluster>());
  REQUIRE(wrapper.as<ExampleCluster>() == cluster);
  REQUIRE_THROWS_AS(wrapper.as<ExampleHit>(), std::runtime_error);
  REQUIRE(wrapper != hit);
}

TEST_CASE("InterfaceType getters", "[basics][interface-types][code-gen]") {
  MutableExampleCluster cluster{};
  cluster.energy(3.14f);

  TypeWithEnergy interfaceType = cluster;
  REQUIRE(interfaceType.energy() == 3.14f);
}

TEST_CASE("InterfaceType extension model", "[interface-types][extension]") {
  using WrapperT = iextension::EnergyInterface;

  auto wrapper = WrapperT::makeEmpty();
  REQUIRE_FALSE(wrapper.isAvailable());

  MutableExampleCluster cluster{};
  cluster.energy(3.14f);
  wrapper = cluster;

  REQUIRE(wrapper.energy() == 3.14f);
  REQUIRE(wrapper.isA<ExampleCluster>());
  REQUIRE(wrapper.as<ExampleCluster>().energy() == 3.14f);

  iextension::MutableAnotherHit hit{};
  hit.energy(4.2f);
  wrapper = hit;

  REQUIRE(wrapper.energy() == 4.2f);
  REQUIRE(wrapper.isA<iextension::AnotherHit>());
  REQUIRE(wrapper.as<iextension::AnotherHit>().energy() == 4.2f);
}
