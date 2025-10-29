#include "core/entities/User.hpp"
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "core/services/UsersManager.hpp"
#include "core/bd/RepositoryFactory.hpp"

#include "fixtures/ConfigFixture.hpp"
#include "fixtures/MediaFixture.hpp"
#include "fixtures/DatabaseFixture.hpp"
#include "fixtures/UserFixture.hpp"

#include "core/bd/UserRepository.hpp"


TEST_SUITE("HISTÓRIA DE USUÁRIO: Segurança e gestão de usuário") {
    ConfigFixture config = ConfigFixture();
    MediaFixture media = MediaFixture();
    UserFixture user_fixture = UserFixture();
    core::RepositoryFactory repo_factory(DatabaseFixture().getDatabase());

    std::unique_ptr<core::UserRepository> user_repo = repo_factory.createUserRepository();


    core::UsersManager userManager(config);

    TEST_CASE("Gerenciar usuários") {

        UserFixture::UserTestMock admin_data = user_fixture.getUserMock("ADMIN_USER");
        UserFixture::UserTestMock normal_data = user_fixture.getUserMock("NORMAL_USER");
        UserFixture::UserTestMock empty_data = user_fixture.getUserMock("EMPTY_USER");

        CHECK(user_repo->count() == 0);

        core::User user(
            admin_data.username,
            admin_data.home_path,
            admin_data.input_path,
            admin_data.uid
        );
        user_repo->save(user);

        std::shared_ptr<core::User> user_ptr = userManager.getUserByUserId(admin_data.id);

        CHECK(user_ptr != nullptr);
        CHECK(user_ptr->getUsername() == admin_data.username);
        CHECK(user_ptr->getUID() == admin_data.uid);
        CHECK(user_repo->count() == 1);

        user = core::User(
            empty_data.username,
            empty_data.home_path,
            empty_data.input_path,
            empty_data.uid
        );
        user_repo->save(user);

        user_ptr = userManager.getUserByUserId(empty_data.id);

        CHECK(user_ptr != nullptr);
        CHECK(user_ptr->getInputPath() == empty_data.username);
        CHECK(user_ptr->getHomePath() == empty_data.home_path);
        CHECK(user_repo->count() == 2);

        user = core::User(
            normal_data.username,
            normal_data.home_path,
            normal_data.input_path,
            normal_data.uid
        );

        auto vector = user_repo->getAll();
        CHECK(vector.size() == 3);
        user_repo->remove(empty_data.id);

        vector = user_repo->getAll();
        CHECK(vector.size() == 2);
    }


}
