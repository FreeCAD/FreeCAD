# FreeCAD AI Policy

With recent developments in the field of AI it is becoming clear that AI technology will have a big impact on Open Source projects including FreeCAD. This document outlines the core project values regarding the technology and acts as the compass that should guide us in decisions concerning AI. With this policy, the FreeCAD developer community **puts people in the first row** while **acknowledging the concerns**. It enforces **contributions with humans in the driver seat** and raises awareness of **fully open and frugal AI**.

We expect from our contributors:

- submissions that they fully understand, that they have thoroughly reviewed and tested,
- disclosure of AI assistance, and
- comments and descriptions made by contributors.

## Putting people in the first row

FreeCAD is a community driven project, and would not exist without the people who helped to shape it throughout the years - that includes both developers and users. That's why we feel that it is important to put the community and the people associated with the project in any way in the first row. This policy acknowledges the harmful sides of AI with respect to communities - this policy tries to avoid that for the FreeCAD community.

## Concerns over AI

There are various concerns surrounding AI. Although many of these concerns are outside of the influence of the FreeCAD project, we want to highlight them so contributors are aware of the concerns around AI.

The concerns include, but are not limited to:

- **privilege:** only large organizations can effectively train and operate high-performance LLMs, limited accessibility to the technology for the unprivileged, and bias in the datasets and outputs in favor of the privileged
- **environmental:** land use, carbon footprint, and water use for cooling of large data centers
- **copyright:** both potential violations of acquiring training data and in the output of AI, and the challenges to claim copyright on raw generative-AI output
- **societal:** exploiting workers for content moderation, affecting the learning process
- **open source projects:** high-volume of low-quality vibe-coded PRs, interaction with AI chatbots, unclear decision process in agentic AI, and increased burden on reviewers and maintainers

### Frugal and fully open AI alternatives

Although the problems with AI regarding open source projects don’t disappear with alternatives, FreeCAD as a project wants to raise awareness of alternatives to the large LLMs that can only be trained by large organizations. Examples are frugal AI initiatives and fully open source AI models such as Apertus that is trained on fully open data in a carbon-neutral and water-neutral data center.

## Humans in the driver seat

FreeCAD is made by humans and for humans, and we believe that it is important for it to stay like that. With contributions we expect to interact with actual people behind their computer, discuss solutions, and look for the best possible outcomes. Reviewers must know that when talking with contributors, they talk to someone who can take responsibility for the contributions, knows how the code works, and who is eager to learn and improve. The project is open to all people: domain experts and new developers alike. With each contribution we value the interaction with the contributor and the community building as much as the improvement to FreeCAD. We want to help the contributor improve their skills and learn how their input can cause real change.

With AI contributions it is almost impossible to support these values and to ensure that they are actually followed. That's why for each contribution we expect it to have human behind the wheel throughout the whole process and be presented by humans, not AI tools. While it won't be possible to verify that fully, we trust our community to do the right thing and contribute in a good faith with these values in mind. In particular, we will not accept pull requests with clearly AI-generated code, commit messages, PR descriptions, and responses to reviewer feedback.

## Concrete measures

### Disclosure

We request disclosure of the used technology in the PR description (in natural language) and with git trailers in the commit messages:
```
Assisted-by: [Model-Family] ([Version/ID])
```

Examples:

```
Assisted-by: gemini-2.5-pro (rev-1)
Assisted-by: GPT-4o-2024-08-06
```

### PR template

We ask the contributor in the Pull Request template to mark a required checkbox that says “This PR is not unverified AI output, I take responsibility for it, and all communication from my side in this PR is done by me personally.”. Don't be afraid that your English is not perfect - we are mostly non-native speakers. We don't recommend using machine translation, but if you do - add original text too.

### “Unverified” label

PRs from outside the Developers group will be marked as "Unverified" to help us find potential violations.  After several successful contributions we will add new contributors to the groups - but don't hesitate to ask us to do it sooner.
