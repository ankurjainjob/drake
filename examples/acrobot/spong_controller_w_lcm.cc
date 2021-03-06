/*
 * An acrobot Spong controller that communicates to acrobot_plang_w_lcm
 * through LCM, implemented by the following diagram system:
 *
 * LcmSubscriberSystem—>
 * AcrobotStateReceiver—>
 * AcrobotSpongController—>
 * AcrobotCommandSender—>
 * LcmPublisherSystem
 *
 */
#include <chrono>
#include <memory>

#include <gflags/gflags.h>

#include "drake/examples/acrobot/acrobot_lcm.h"
#include "drake/examples/acrobot/spong_controller.h"
#include "drake/lcm/drake_lcm.h"
#include "drake/lcmt_acrobot_u.hpp"
#include "drake/lcmt_acrobot_x.hpp"
#include "drake/systems/framework/diagram_builder.h"
#include "drake/systems/framework/leaf_system.h"
#include "drake/systems/lcm/lcm_publisher_system.h"
#include "drake/systems/lcm/lcm_subscriber_system.h"

DEFINE_double(time_limit_sec, std::numeric_limits<double>::infinity(),
              "Number of seconds to run (default: infinity).");

namespace drake {
namespace examples {
namespace acrobot {
namespace {

int DoMain() {
  drake::systems::DiagramBuilder<double> builder;
  const std::string channel_x = "acrobot_xhat";
  const std::string channel_u = "acrobot_u";
  lcm::DrakeLcm lcm;

  // -----------------controller--------------------------------------
  // Create state receiver.
  auto state_sub = builder.AddSystem(
      systems::lcm::LcmSubscriberSystem::Make<lcmt_acrobot_x>(channel_x, &lcm));
  auto state_receiver = builder.AddSystem<AcrobotStateReceiver>();
  builder.Connect(state_sub->get_output_port(),
                  state_receiver->get_input_port(0));

  // Create command sender.
  auto command_pub = builder.AddSystem(
      systems::lcm::LcmPublisherSystem::Make<lcmt_acrobot_u>(channel_u, &lcm));
  auto command_sender = builder.AddSystem<AcrobotCommandSender>();
  builder.Connect(command_sender->get_output_port(0),
                  command_pub->get_input_port());

  auto controller = builder.AddSystem<AcrobotSpongController>();
  builder.Connect(controller->get_output_port(0),
                  command_sender->get_input_port(0));
  builder.Connect(state_receiver->get_output_port(0),
                  controller->get_input_port(0));

  auto diagram = builder.Build();
  auto context = diagram->CreateDefaultContext();

  lcm.StartReceiveThread();

  using clock = std::chrono::system_clock;
  const auto& start_time = clock::now();
  const auto& max_duration =
      std::chrono::duration<double>(FLAGS_time_limit_sec);
  while ((clock::now() - start_time) <= max_duration) {
    const systems::Context<double>& pub_context =
        diagram->GetSubsystemContext(*command_pub, *context);
    command_pub->Publish(pub_context);
  }

  lcm.StopReceiveThread();
  return 0;
}

}  // namespace
}  // namespace acrobot
}  // namespace examples
}  // namespace drake

int main(int argc, char* argv[]) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  return drake::examples::acrobot::DoMain();
}
