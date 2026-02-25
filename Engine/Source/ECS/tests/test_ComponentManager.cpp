#include <gtest/gtest.h>

#include "ECS/Component/ComponentManager.h"

namespace glaze::ecs::tests {
	struct TestPosition {
		float x, y;
	};

	struct TestVelocity {
		static constexpr auto STORAGE_TYPE = StorageType::SparseSet;
		float x, y;
	};

	struct TestComponentManager : testing::Test {
	protected:
		ComponentManager manager;
	};

	TEST(ComponentStorageType, DefaultIsTable) {
		EXPECT_EQ(get_storage_type<TestPosition>(), StorageType::Table);
		EXPECT_EQ(get_storage_type<TestPosition&>(), StorageType::Table);
		EXPECT_EQ(get_storage_type<const TestPosition&>(), StorageType::Table);
	}

	TEST(ComponentStorageType, RespectsCustomStorageType) {
		EXPECT_EQ(get_storage_type<TestVelocity>(), StorageType::SparseSet);
		EXPECT_EQ(get_storage_type<TestVelocity&>(), StorageType::SparseSet);
		EXPECT_EQ(get_storage_type<const TestVelocity&>(), StorageType::SparseSet);
	}

	TEST(ComponentDesc, OfUsesExpectedStorageTypeAndTypeInfo) {
		constexpr auto pos_desc = ComponentDesc::of<TestPosition>();
		constexpr auto vel_desc = ComponentDesc::of<TestVelocity>();

		EXPECT_EQ(pos_desc.storage_type(), StorageType::Table);
		EXPECT_EQ(vel_desc.storage_type(), StorageType::SparseSet);

		EXPECT_EQ(pos_desc.type_info(), utils::TypeInfo::of<TestPosition>());
		EXPECT_EQ(vel_desc.type_info(), utils::TypeInfo::of<TestVelocity>());
	}

	TEST_F(TestComponentManager, ComponentIdReturnsNullForUnregistered) {
		EXPECT_EQ(manager.component_id<TestPosition>(), utils::null_id);
		EXPECT_EQ(manager.component_id<TestVelocity>(), utils::null_id);
		EXPECT_EQ(manager.component_id(utils::TypeInfo::of<TestPosition>()), utils::null_id);
	}

	TEST_F(TestComponentManager, Empty) {
		EXPECT_TRUE(manager.empty());
		EXPECT_EQ(manager.size(), 0);
	}

	TEST_F(TestComponentManager, RegisterSameComponents) {
		const auto position_id_0 = manager.register_component<TestPosition>();
		const auto position_id_1 = manager.register_component<TestPosition&>();
		const auto position_id_2 = manager.register_component<const TestPosition&>();
		EXPECT_EQ(manager.size(), 1);
		EXPECT_EQ(position_id_0, position_id_1);
		EXPECT_EQ(position_id_0, position_id_2);
		EXPECT_EQ(position_id_1, position_id_2);
	}

	TEST_F(TestComponentManager, RegisterDifferentComponents) {
		const auto position_id = manager.register_component<TestPosition>();
		const auto velocity_id = manager.register_component<TestVelocity>();
		EXPECT_EQ(manager.size(), 2);
		EXPECT_NE(position_id, velocity_id);
	}

	TEST_F(TestComponentManager, ComponentId) {
		const auto registered_position_id = manager.register_component<TestPosition>();
		const auto registered_velocity_id = manager.register_component<TestVelocity>();

		const auto position_id = manager.component_id<TestPosition>();
		const auto velocity_id = manager.component_id<TestVelocity>();

		EXPECT_EQ(registered_position_id, position_id);
		EXPECT_EQ(registered_velocity_id, velocity_id);
	}

	TEST_F(TestComponentManager, GetMeta) {
		const auto position_id = manager.register_component<TestPosition>();

		const auto* meta = manager.get_meta(position_id);
		ASSERT_NE(meta, nullptr);
		EXPECT_EQ(meta->id(), position_id);
		EXPECT_EQ(meta->storage_type(), StorageType::Table);
		EXPECT_EQ(meta->type_info(), utils::TypeInfo::of<TestPosition>());
		EXPECT_FALSE(meta->name().empty());

		constexpr auto invalid_id = ComponentId::from_index(1024);
		EXPECT_EQ(manager.get_meta(invalid_id), nullptr);
	}

	TEST_F(TestComponentManager, GetDesc) {
		const auto velocity_id = manager.register_component<TestVelocity>();

		const auto* desc = manager.get_desc(velocity_id);
		ASSERT_NE(desc, nullptr);
		EXPECT_EQ(desc->storage_type(), StorageType::SparseSet);
		EXPECT_EQ(desc->type_info(), utils::TypeInfo::of<TestVelocity>());
		EXPECT_FALSE(desc->name().empty());

		constexpr auto invalid_id = ComponentId::from_index(1024);
		EXPECT_EQ(manager.get_desc(invalid_id), nullptr);
	}

	TEST_F(TestComponentManager, GetName) {
		const auto velocity_id = manager.register_component<TestVelocity>();

		const auto name = manager.get_name(velocity_id);
		ASSERT_TRUE(name.has_value());
		EXPECT_EQ(*name, typeid(TestVelocity).name());

		constexpr auto invalid_id = ComponentId::from_index(1024);
		EXPECT_FALSE(manager.get_name(invalid_id).has_value());
	}

	TEST_F(TestComponentManager, IsIdValid) {
		const auto position_id = manager.register_component<TestPosition>();
		const auto velocity_id = manager.register_component<TestVelocity>();
		EXPECT_TRUE(manager.is_id_valid(position_id));
		EXPECT_TRUE(manager.is_id_valid(velocity_id));

		constexpr ComponentId possibly_invalid_id{1024};
		EXPECT_FALSE(manager.is_id_valid(possibly_invalid_id));
	}
}