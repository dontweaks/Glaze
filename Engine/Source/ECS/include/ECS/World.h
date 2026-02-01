#pragma once

#include "Bundle/Bundle.h"
#include "Component/Component.h"

#include "Entity.h"

namespace glaze::ecs {
	struct World {
		Entity create_empty();

		template<Bundle B>
		Entity create_entity(B&& bundle) {
			return Entity{EntityIndex{0}};
		}

		template<Component ... Cs> requires (sizeof ... (Cs) > 0)
		Entity create_entity(Cs&& ... cs) {
			return create_entity(TupleBundle(cs...));
		}

		bool destroy_entity(const Entity entity);

		template<Bundle B>
		void add_bundle(const Entity entity, B&& bundle);

		template<Component ... Cs> requires (sizeof ... (Cs) > 0)
		void add_components(const Entity entity, Cs&& ... cs) {
			return add_bundle(entity, TupleBundle{ std::forward_as_tuple(std::forward<Cs>(cs)...) });
		}

		template<Bundle B>
		bool remove_bundle(const Entity entity);

		template<Component ... Cs> requires (sizeof ... (Cs) > 0)
		bool remove_components(const Entity entity) {
			return remove_bundle<TupleBundle<Cs...>>(entity);
		}

		template<Bundle B>
		void register_bundle();

		template<Component ... Cs> requires (sizeof ... (Cs) > 0)
		void register_components() {
			register_bundle<TupleBundle<Cs...>>();
		}

	private:
		WorldId m_id{0};
	};
}